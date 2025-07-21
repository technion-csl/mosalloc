#! /usr/bin/env python3
import sys
import warnings
from subprocess import call
import os
import argparse
import subprocess
 
def parse_arguments():
    parser = argparse.ArgumentParser(description='A tool to run applications while\
             pre-loading mosalloc library to intercept allocation calls and\
             redirect them to pre-allocated regions backed with mixed pages sizes')
    parser.add_argument('-z', '--analyze', action='store_true',
                        help="analyze the pool sizes and write them to new file called mosalloc_hpbrs.csv.<pid>")
    parser.add_argument('-d', '--debug', action='store_true',
                        help="run in debug mode and don't run preparation scripts (e.g., disable THP)")
    parser.add_argument('-l', '--library', default='src/morecore/lib_morecore.so',
                        help="mosalloc library path to preload.")
    parser.add_argument('-cpf', '--configuration_pools_file', required=True,
                        help="path to csv file with pools configuration")
    parser.add_argument('dispatch_program', help="program to execute")
    parser.add_argument('dispatch_args', nargs=argparse.REMAINDER,
                        help="program arguments")
    
    args = parser.parse_args()

    # validate the command-line arguments
    if not os.path.isfile(args.library):
        sys.exit("Error: the mosalloc library cannot be found")
    if not os.path.isfile(args.configuration_pools_file):
        sys.exit("Error: the configuration pools file cannot be found")
    return args

def update_config_file():
    config_file = 'configuration.txt'
    current_config = ' '.join(sys.argv[1:]) + '\n'
    import os.path
    if os.path.isfile(config_file):
        with open(config_file, 'r') as file:
            config_file_data = file.readlines()
        config_file_data = ' '.join(config_file_data)
        if not current_config == config_file_data:
            raise RuntimeError(
                'existing configuration file is different than current one:\n'
                + 'existing configuration file: ' + config_file_data + '\n'
                + 'current run configuration: ' + current_config)
        else:
            print('current configuration already exists and identical, skipping running again')
            sys.exit(0)
    else:
        with open(config_file, 'w+') as file:
            file.write(current_config)

from memory_layout_config import *
def run_benchmark(environ: dict, config_file: str, dispatch_program: str, dispatch_args: list, debug=False):
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

    try:
        if not debug:
            scripts_home_directory = sys.path[0]
            reserve_huge_pages_script = scripts_home_directory + "/reserveHugePages.sh"
            # set THP and reserve hugepages before start running the workload
            subprocess.check_call(['flock', '-x', reserve_huge_pages_script,
                                   reserve_huge_pages_script, '--huge2mb=' + str(hugepages_2mb_count),
                                   '--huge1gb=' + str(hugepages_1gb_count)])

        command_line = [dispatch_program] + dispatch_args
        print(f'Mosalloc: start running {command_line}')
        p = subprocess.Popen(command_line, env=environ, shell=False)
        p.wait()
        sys.exit(p.returncode)
    except Exception as e:
        raise e

from legacy_memory_region import MemoryRegion
def legacy_run_benchmark(environ, config_file: str, dispatch_program: str, dispatch_args: list, debug=False):
    anon_region = MemoryRegion(config_file, 'mmap')
    brk_region = MemoryRegion(config_file, 'brk')
    # reserve an additional large/huge page so we can pad the pools with this
    # extra page and allow proper alignment of large/huge pages inside the pools
    large_pages = anon_region.get_num_of_large_pages() + brk_region.get_num_of_large_pages()
    large_pages = large_pages + 1 if large_pages > 0 else large_pages
    huge_pages = anon_region.get_num_of_huge_pages() + brk_region.get_num_of_huge_pages()
    huge_pages = huge_pages + 1 if huge_pages > 0 else huge_pages

    try:
        if not debug:
            scripts_home_directory = sys.path[0]
            reserve_huge_pages_script = scripts_home_directory + "/reserveHugePages.sh"
            # set THP and reserve hugepages before start running the workload
            subprocess.check_call(['flock', '-x', reserve_huge_pages_script,
                                   reserve_huge_pages_script, '-l' + str(large_pages),
                                   '-h' + str(huge_pages)])

        command_line = [dispatch_program] + dispatch_args
        print(f'Mosalloc: start running {command_line}')
        p = subprocess.Popen(command_line, env=environ, shell=False)
        p.wait()
        sys.exit(p.returncode)
    except Exception as e:
        raise e

import pandas as pd
def is_legacy_config_file(config_file: str):
    df = pd.read_csv(config_file)
    config_file_cols = df.columns
    
    old_cols = ['type', 'pageSize', 'startOffset', 'endOffset']
    legacy_config = True
    for c in old_cols:
        if c not in config_file_cols:
            legacy_config = False
            break
    
    new_cols = ['pool_type', 'pool_size', 'ragions_list_2mb', 'offset_2mb', 'ragions_list_1gb', 'offset_1gb']
    new_config = True
    for c in new_cols:
        if c not in config_file_cols:
            new_config = False
            break
    
    if new_config:
        return False
    if legacy_config:
        return True
    assert False, f'{config_file} format is neither in legacy nor in new format!'

if __name__ == "__main__":
    args = parse_arguments()
        
    # build the environment variables
    environ = {"HPC_CONFIGURATION_FILE": args.configuration_pools_file,
            "HPC_MMAP_FIRST_FIT_LIST_SIZE": str(int(Size("1MB"))),
            "HPC_FILE_BACKED_FIRST_FIT_LIST_SIZE": str(int(Size("10KB")))}

    if args.analyze:
        environ["HPC_ANALYZE_HPBRS"] = "1"

    environ.update(os.environ)

    # update the LD_PRELOAD to include our library besides others if exist
    ld_preload = os.environ.get("LD_PRELOAD")
    if ld_preload is None:
        environ["LD_PRELOAD"] = args.library
    else:
        environ["LD_PRELOAD"] = ld_preload + ':' + args.librarySS

    # check if configuration.txt file exists and if so compare with current parameters
    #update_config_file()  #TODO: Alon&Yaron code, should be tested
    
    
    legacy_mode = is_legacy_config_file(args.configuration_pools_file)
    if legacy_mode:
        # print deprecation warning
        msg = f'\n\t using old {sys.argv[0]} format is deprecated and will be removed in the future.' + '\n' + \
            '\t please start using the new format by specifying the temporary argument "--new"'
        # warnings.warn(msg, DeprecationWarning)
        
        # Run the benchmark in legacy format
        legacy_run_benchmark(environ, args.configuration_pools_file, args.dispatch_program, args.dispatch_args, args.debug)
    else:
        # dispatch the program with the environment we just set
        run_benchmark(environ, args.configuration_pools_file, args.dispatch_program, args.dispatch_args, args.debug)
