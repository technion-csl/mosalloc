#!/usr/bin/env bash
set -euo pipefail

print_usage_and_exit() {
    cat <<'EOF'
Usage:
  reserveHugePages.sh acquire --owner=ID [-n NODE] --huge2mb=N --huge1gb=M
  reserveHugePages.sh release --owner=ID [-n NODE]
  reserveHugePages.sh status  [-n NODE]
  reserveHugePages.sh reset   [-n NODE]

Notes:
- acquire adds this owner's reservation to the global per-node total.
- release removes exactly this owner's reservation.
- owner must be unique per job, e.g. pair_layout_repeat.
- state is tracked under /tmp/reserveHugePages-state.
EOF
    exit 1
}

get_default_node() {
    numactl -s | awk '/nodebind/ {print $2; exit}'
}

get_huge2mb_count() {
    cat "$huge2mb_pages_file"
}

get_huge1gb_count() {
    if [[ -f "$huge1gb_pages_file" ]]; then
        cat "$huge1gb_pages_file"
    else
        echo 0
    fi
}

set_huge2mb_count() {
    echo "Trying to allocate $1 huge2mb pages"
    sudo bash -c "echo $1 > '$huge2mb_pages_file'"
}

set_huge1gb_count() {
    if [[ -f "$huge1gb_pages_file" ]]; then
        echo "Trying to allocate $1 huge1gb pages"
        sudo bash -c "echo $1 > '$huge1gb_pages_file'"
    else
        if (( $1 > 0 )); then
            echo "WARNING: 1GB hugepages are not supported/enabled"
            echo "WARNING: $huge1gb_pages_file does not exist. Skipping reserving 1GB hugepages"
        fi
    fi
}

disable_thp() {
    local thp
    thp="$(cat /sys/kernel/mm/transparent_hugepage/enabled)"
    if [[ "$thp" != "always madvise [never]" ]]; then
        echo "Disable Transparent Huge Pages (set THP to never)"
        sudo bash -c "echo never > /sys/kernel/mm/transparent_hugepage/enabled"
    fi
}

set_overcommit_memory() {
    local overcommit
    overcommit="$(cat /proc/sys/vm/overcommit_memory)"
    if (( overcommit != 1 )); then
        echo "Enable overcommit memory"
        sudo bash -c "echo 1 > /proc/sys/vm/overcommit_memory"
    fi
}

print_actual_status() {
    echo "  # of 2MB pages(node${node}) == $(get_huge2mb_count)"
    echo "  # of 1GB pages(node${node}) == $(get_huge1gb_count)"
}

load_totals() {
    total_2mb=0
    total_1gb=0
    if [[ -f "$totals_file" ]]; then
        # shellcheck disable=SC1090
        source "$totals_file"
    fi
}

write_totals() {
    cat > "$totals_file" <<EOF
total_2mb=$1
total_1gb=$2
EOF
}

load_owner_reservation() {
    owner_2mb=0
    owner_1gb=0
    if [[ -f "$owner_file" ]]; then
        # shellcheck disable=SC1090
        source "$owner_file"
    fi
}

write_owner_reservation() {
    cat > "$owner_file" <<EOF
owner_2mb=$1
owner_1gb=$2
EOF
}

apply_target_counts() {
    local target_2mb="$1"
    local target_1gb="$2"
    local before_2mb before_1gb after_2mb after_1gb

    before_2mb="$(get_huge2mb_count)"
    before_1gb="$(get_huge1gb_count)"

    echo "Reserving hugepages..."
    echo "Currently:"
    print_actual_status

    # Keep the original ordering logic:
    # if 2MB needs to go down, shrink it first to free room/contiguity;
    # otherwise try 1GB first, then 2MB.
    if (( before_2mb > target_2mb )); then
        set_huge2mb_count "$target_2mb"
        set_huge1gb_count "$target_1gb"
    else
        set_huge1gb_count "$target_1gb"
        set_huge2mb_count "$target_2mb"
    fi

    after_2mb="$(get_huge2mb_count)"
    after_1gb="$(get_huge1gb_count)"

    echo "Updated:"
    print_actual_status
    echo "--------------------------------------------"

    if (( after_2mb >= target_2mb && after_1gb >= target_1gb )); then
        echo "Huge pages were set correctly"
        return 0
    fi

    echo "ERROR: could not reserve requested totals (target_2mb=$target_2mb, target_1gb=$target_1gb)" >&2
    echo "Attempting rollback to previous totals: 2MB=$before_2mb 1GB=$before_1gb" >&2

    if (( after_2mb > before_2mb )); then
        set_huge2mb_count "$before_2mb" || true
        set_huge1gb_count "$before_1gb" || true
    else
        set_huge1gb_count "$before_1gb" || true
        set_huge2mb_count "$before_2mb" || true
    fi

    return 1
}

require_owner() {
    if [[ -z "${owner:-}" ]]; then
        echo "ERROR: --owner is required for $mode" >&2
        exit 1
    fi
}

mode="${1:-}"
[[ -n "$mode" ]] || print_usage_and_exit
shift || true

node=""
huge2mb=""
huge1gb=""
owner=""

getopt --test > /dev/null
if (( $? != 4 )); then
    echo "Error: this system has an old version of getopt." >&2
    exit 1
fi

short_options=n:l:h:o:
long_options=node:,huge2mb:,huge1gb:,owner:

parsed="$(getopt \
    --options="$short_options" \
    --longoptions="$long_options" \
    --name "$0" \
    -- "$@")" || {
    echo "Error: getopt couldn't parse the command-line arguments." >&2
    exit 1
}
eval set -- "$parsed"

while true; do
    case "$1" in
        -n|--node)    node="$2"; shift 2 ;;
        -l|--huge2mb) huge2mb="$2"; shift 2 ;;
        -h|--huge1gb) huge1gb="$2"; shift 2 ;;
        -o|--owner)   owner="$2"; shift 2 ;;
        --) shift; break ;;
        *) print_usage_and_exit ;;
    esac
done

case "$mode" in
    acquire|release|status|reset) ;;
    *) print_usage_and_exit ;;
esac

if [[ -z "$node" ]]; then
    node="$(get_default_node)"
    echo "Using node${node}..."
fi

proc_path="/sys/devices/system/node/node${node}/hugepages"
huge2mb_pages_file="${proc_path}/hugepages-2048kB/nr_hugepages"
huge1gb_pages_file="${proc_path}/hugepages-1048576kB/nr_hugepages"

state_root="/tmp/reserveHugePages-state/node${node}"
owners_dir="${state_root}/owners"
totals_file="${state_root}/totals.env"
lock_file="${state_root}/lock"

mkdir -p "$owners_dir"
exec 9>"$lock_file"
flock 9

echo "---------------- DEBUG INFO ----------------"
set_overcommit_memory
disable_thp

case "$mode" in
    status)
        echo "Actual kernel hugepage status:"
        print_actual_status
        load_totals
        echo "Tracked totals:"
        echo "  total_2mb=$total_2mb"
        echo "  total_1gb=$total_1gb"
        exit 0
        ;;
    reset)
        echo "Resetting tracked state and kernel hugepage counts for node${node}"
        apply_target_counts 0 0
        rm -f "$totals_file"
        rm -rf "$owners_dir"
        mkdir -p "$owners_dir"
        exit 0
        ;;
    acquire)
        require_owner
        [[ -n "$huge2mb" && -n "$huge1gb" ]] || print_usage_and_exit
        owner_file="${owners_dir}/${owner}.env"

        load_totals
        load_owner_reservation

        if [[ -f "$owner_file" ]]; then
            echo "Owner '$owner' already has a reservation: 2MB=$owner_2mb 1GB=$owner_1gb"
            if (( owner_2mb == huge2mb && owner_1gb == huge1gb )); then
                echo "Acquire is idempotent for this owner; nothing to do."
                exit 0
            else
                echo "ERROR: owner '$owner' already exists with different reservation" >&2
                exit 1
            fi
        fi

        new_total_2mb=$(( total_2mb + huge2mb ))
        new_total_1gb=$(( total_1gb + huge1gb ))

        echo "Owner '$owner' acquiring: +${huge2mb} huge2mb, +${huge1gb} huge1gb"
        echo "Tracked totals before: 2MB=$total_2mb 1GB=$total_1gb"
        echo "Tracked totals after : 2MB=$new_total_2mb 1GB=$new_total_1gb"

        apply_target_counts "$new_total_2mb" "$new_total_1gb"
        write_totals "$new_total_2mb" "$new_total_1gb"
        write_owner_reservation "$huge2mb" "$huge1gb"
        exit 0
        ;;
    release)
        require_owner
        owner_file="${owners_dir}/${owner}.env"

        load_totals
        if [[ ! -f "$owner_file" ]]; then
            echo "ERROR: owner '$owner' has no reservation to release" >&2
            exit 1
        fi
        load_owner_reservation

        new_total_2mb=$(( total_2mb - owner_2mb ))
        new_total_1gb=$(( total_1gb - owner_1gb ))

        (( new_total_2mb < 0 )) && new_total_2mb=0
        (( new_total_1gb < 0 )) && new_total_1gb=0

        echo "Owner '$owner' releasing: -${owner_2mb} huge2mb, -${owner_1gb} huge1gb"
        echo "Tracked totals before: 2MB=$total_2mb 1GB=$total_1gb"
        echo "Tracked totals after : 2MB=$new_total_2mb 1GB=$new_total_1gb"

        apply_target_counts "$new_total_2mb" "$new_total_1gb"
        write_totals "$new_total_2mb" "$new_total_1gb"
        rm -f "$owner_file"
        exit 0
        ;;
esac