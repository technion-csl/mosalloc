#! /usr/bin/env python3
import sys
import warnings
import os
import argparse
import subprocess
import time
import socket
from pathlib import Path

import pandas as pd

from memory_layout_config import *
from legacy_memory_region import MemoryRegion


def parse_arguments():
    parser = argparse.ArgumentParser(
        description='A tool to run applications while pre-loading mosalloc library '
                    'to intercept allocation calls and redirect them to pre-allocated '
                    'regions backed with mixed page sizes'
    )
    parser.add_argument(
        '-z', '--analyze', action='store_true',
        help="analyze the pool sizes and write them to new file called mosalloc_hpbrs.csv.<pid>"
    )
    parser.add_argument(
        '-d', '--debug', action='store_true',
        help="run in debug mode and don't run preparation scripts (e.g., disable THP)"
    )
    parser.add_argument(
        '-l', '--library', default='src/morecore/lib_morecore.so',
        help="mosalloc library path to preload."
    )
    parser.add_argument(
        '-cpf', '--configuration_pools_file', required=True,
        help="path to csv file with pools configuration"
    )
    parser.add_argument('dispatch_program', help="program to execute")
    parser.add_argument('dispatch_args', nargs=argparse.REMAINDER, help="program arguments")

    args = parser.parse_args()

    if not os.path.isfile(args.library):
        sys.exit("Error: the mosalloc library cannot be found")
    if not os.path.isfile(args.configuration_pools_file):
        sys.exit("Error: the configuration pools file cannot be found")
    return args


def update_config_file():
    config_file = 'configuration.txt'
    current_config = ' '.join(sys.argv[1:]) + '\n'
    if os.path.isfile(config_file):
        with open(config_file, 'r') as file:
            config_file_data = file.readlines()
        config_file_data = ' '.join(config_file_data)
        if current_config != config_file_data:
            raise RuntimeError(
                'existing configuration file is different than current one:\n'
                + 'existing configuration file: ' + config_file_data + '\n'
                + 'current run configuration: ' + current_config
            )
        else:
            print('current configuration already exists and identical, skipping running again')
            sys.exit(0)
    else:
        with open(config_file, 'w+') as file:
            file.write(current_config)


def is_legacy_config_file(config_file: str):
    df = pd.read_csv(config_file)
    config_file_cols = df.columns

    old_cols = ['type', 'pageSize', 'startOffset', 'endOffset']
    legacy_config = all(c in config_file_cols for c in old_cols)

    new_cols = [
        'pool_type', 'pool_size',
        'ragions_list_2mb', 'offset_2mb',
        'ragions_list_1gb', 'offset_1gb'
    ]
    new_config = all(c in config_file_cols for c in new_cols)

    if new_config:
        return False
    if legacy_config:
        return True
    assert False, f'{config_file} format is neither in legacy nor in new format!'


def get_scripts_home_directory() -> str:
    return sys.path[0]


def get_reserve_huge_pages_script() -> str:
    return os.path.join(get_scripts_home_directory(), "reserveHugePages.sh")


def build_hugepage_owner(config_file: str, dispatch_program: str) -> str:
    """
    Build a unique owner id for reserveHugePages acquire/release.

    Priority:
    1. MOSALLOC_HUGEPAGES_OWNER env if explicitly provided
    2. auto-generated id based on cwd, config, program, pid, hostname, timestamp
    """
    explicit = os.environ.get("MOSALLOC_HUGEPAGES_OWNER")
    if explicit:
        return explicit

    cwd_base = Path(os.getcwd()).name
    cfg_base = Path(config_file).stem
    prog_base = Path(dispatch_program).name
    host = socket.gethostname().split('.')[0]
    pid = os.getpid()
    ts = int(time.time() * 1000)

    owner = f"{cwd_base}_{cfg_base}_{prog_base}_{host}_{pid}_{ts}"
    # keep it filesystem-friendly
    owner = owner.replace('/', '_').replace(' ', '_').replace(':', '_')
    return owner


def acquire_hugepages(
    hugepages_2mb_count: int,
    hugepages_1gb_count: int,
    owner: str,
    debug: bool = False,
) -> bool:
    """
    Reserve hugepages for this run using acquire semantics.
    Returns True if acquire was attempted and succeeded, False if skipped.
    """
    if debug:
        return False

    reserve_huge_pages_script = get_reserve_huge_pages_script()

    # No need to call acquire when both are zero.
    if hugepages_2mb_count <= 0 and hugepages_1gb_count <= 0:
        return False

    cmd = [
        reserve_huge_pages_script,
        "acquire",
        f"--owner={owner}",
        f"--huge2mb={hugepages_2mb_count}",
        f"--huge1gb={hugepages_1gb_count}",
    ]

    print(f"Mosalloc: acquiring hugepages with owner={owner}")
    print(f"Mosalloc: command={' '.join(cmd)}")
    subprocess.check_call(cmd)
    return True


def release_hugepages(owner: str, debug: bool = False) -> None:
    """
    Release hugepages for this run using release semantics.
    Best effort in finally: do not hide the original benchmark failure.
    """
    if debug:
        return

    reserve_huge_pages_script = get_reserve_huge_pages_script()
    cmd = [
        reserve_huge_pages_script,
        "release",
        f"--owner={owner}",
    ]

    print(f"Mosalloc: releasing hugepages with owner={owner}")
    print(f"Mosalloc: command={' '.join(cmd)}")
    try:
        subprocess.check_call(cmd)
    except Exception as e:
        print(f"WARNING: failed to release hugepages for owner={owner}: {e}", file=sys.stderr)


def run_with_optional_hugepages(
    environ: dict,
    config_file: str,
    dispatch_program: str,
    dispatch_args: list,
    hugepages_2mb_count: int,
    hugepages_1gb_count: int,
    debug: bool = False,
) -> int:
    """
    Common execution path:
    - acquire hugepages
    - launch benchmark
    - release hugepages in finally
    """
    owner = build_hugepage_owner(config_file, dispatch_program)
    acquired = False
    p = None

    try:
        acquired = acquire_hugepages(
            hugepages_2mb_count=hugepages_2mb_count,
            hugepages_1gb_count=hugepages_1gb_count,
            owner=owner,
            debug=debug,
        )

        command_line = [dispatch_program] + dispatch_args
        print(f'Mosalloc: start running {command_line}')
        p = subprocess.Popen(command_line, env=environ, shell=False)
        p.wait()
        return p.returncode

    finally:
        if acquired:
            release_hugepages(owner=owner, debug=debug)


def run_benchmark(
    environ: dict,
    config_file: str,
    dispatch_program: str,
    dispatch_args: list,
    debug: bool = False
):
    memory_layout = MemoryLayout(config_file)
    memory_layout.validate_pools()

    legacy_df = memory_layout.build_legacy_configuration_layout()
    legacy_df.to_csv('legacy_config.csv', index=False)

    # workaround until Mosalloc config parser moves to parse new format
    environ.update({'HPC_CONFIGURATION_FILE': 'legacy_config.csv'})

    # reserve an additional large/huge page so we can pad the pools with this
    # extra page and allow proper alignment of large/huge pages inside the pools
    hugepages_2mb_count = memory_layout.get_total_hugepages_2mb()
    hugepages_2mb_count = hugepages_2mb_count + 1 if hugepages_2mb_count > 0 else hugepages_2mb_count

    hugepages_1gb_count = memory_layout.get_total_hugepages_1gb()
    hugepages_1gb_count = hugepages_1gb_count + 1 if hugepages_1gb_count > 0 else hugepages_1gb_count

    rc = run_with_optional_hugepages(
        environ=environ,
        config_file=config_file,
        dispatch_program=dispatch_program,
        dispatch_args=dispatch_args,
        hugepages_2mb_count=hugepages_2mb_count,
        hugepages_1gb_count=hugepages_1gb_count,
        debug=debug,
    )
    sys.exit(rc)


def legacy_run_benchmark(
    environ,
    config_file: str,
    dispatch_program: str,
    dispatch_args: list,
    debug: bool = False
):
    anon_region = MemoryRegion(config_file, 'mmap')
    brk_region = MemoryRegion(config_file, 'brk')

    # reserve an additional large/huge page so we can pad the pools with this
    # extra page and allow proper alignment of large/huge pages inside the pools
    large_pages = anon_region.get_num_of_large_pages() + brk_region.get_num_of_large_pages()
    large_pages = large_pages + 1 if large_pages > 0 else large_pages

    huge_pages = anon_region.get_num_of_huge_pages() + brk_region.get_num_of_huge_pages()
    huge_pages = huge_pages + 1 if huge_pages > 0 else huge_pages

    rc = run_with_optional_hugepages(
        environ=environ,
        config_file=config_file,
        dispatch_program=dispatch_program,
        dispatch_args=dispatch_args,
        hugepages_2mb_count=large_pages,
        hugepages_1gb_count=huge_pages,
        debug=debug,
    )
    sys.exit(rc)


if __name__ == "__main__":
    args = parse_arguments()

    # build the environment variables
    environ = {
        "HPC_CONFIGURATION_FILE": args.configuration_pools_file,
        "HPC_MMAP_FIRST_FIT_LIST_SIZE": str(int(Size("1MB"))),
        "HPC_FILE_BACKED_FIRST_FIT_LIST_SIZE": str(int(Size("10KB"))),
    }

    if args.analyze:
        environ["HPC_ANALYZE_HPBRS"] = "1"

    environ.update(os.environ)

    # update LD_PRELOAD to include mosalloc library
    ld_preload = os.environ.get("LD_PRELOAD")
    if ld_preload is None:
        environ["LD_PRELOAD"] = args.library
    else:
        environ["LD_PRELOAD"] = ld_preload + ':' + args.library

    # check if configuration.txt file exists and if so compare with current parameters
    # update_config_file()  # TODO: Alon&Yaron code, should be tested

    legacy_mode = is_legacy_config_file(args.configuration_pools_file)
    if legacy_mode:
        msg = (
            f'\n\t using old {sys.argv[0]} format is deprecated and will be removed in the future.\n'
            '\t please start using the new format by specifying the temporary argument "--new"'
        )
        # warnings.warn(msg, DeprecationWarning)

        legacy_run_benchmark(
            environ,
            args.configuration_pools_file,
            args.dispatch_program,
            args.dispatch_args,
            args.debug,
        )
    else:
        run_benchmark(
            environ,
            args.configuration_pools_file,
            args.dispatch_program,
            args.dispatch_args,
            args.debug,
        )