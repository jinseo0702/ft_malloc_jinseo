#!/bin/sh
set -u

if [ "$#" -ne 2 ]; then
  echo "usage: $0 REPOSITORY_ROOT RAW_OUTPUT_DIRECTORY" >&2
  exit 2
fi

audit_repo_root=$(CDPATH= cd -- "$1" 2>/dev/null && pwd)
if [ -z "${audit_repo_root:-}" ]; then
  echo "repository root is not accessible: $1" >&2
  exit 2
fi

audit_raw_dir=$2
mkdir -p -- "$audit_raw_dir"
audit_tmp_dir=$(mktemp -d /tmp/ft_malloc_profile_b_gate.XXXXXX) || exit 2
trap 'rm -rf -- "$audit_tmp_dir"' EXIT HUP INT TERM

cp -a -- "$audit_repo_root/." "$audit_tmp_dir/repo"
audit_copy_root=$audit_tmp_dir/repo

rg -n '\bmain[[:space:]]*\(' "$audit_repo_root/src" "$audit_repo_root/include" > "$audit_raw_dir/production_main_matches.txt" 2>&1 || :
rg -n 'Elf(32|64)_[[:alnum:]_]+|<elf\.h>|<gelf\.h>|\.symtab|\.dynsym|SHT_SYMTAB|SHT_DYNSYM' "$audit_repo_root/src" "$audit_repo_root/include" > "$audit_raw_dir/elf_parser_signal_matches.txt" 2>&1 || :
audit_main_count=$(wc -l < "$audit_raw_dir/production_main_matches.txt" | tr -d ' ')
audit_parser_count=$(wc -l < "$audit_raw_dir/elf_parser_signal_matches.txt" | tr -d ' ')

if make -C "$audit_copy_root" > "$audit_raw_dir/build.stdout" 2> "$audit_raw_dir/build.stderr"; then
  audit_build_exit=0
else
  audit_build_exit=$?
fi

audit_host_type=$(uname -m)_$(uname -s)
audit_artifact=$audit_copy_root/libft_malloc_${audit_host_type}.so
audit_artifact_present=false
audit_artifact_is_shared_object=false

if [ -f "$audit_artifact" ]; then
  audit_artifact_present=true
  file -b -- "$audit_artifact" > "$audit_raw_dir/artifact_file.txt"
  readelf -h -- "$audit_artifact" > "$audit_raw_dir/artifact_elf_header.txt"
  nm -D --defined-only -- "$audit_artifact" > "$audit_raw_dir/artifact_dynamic_symbols.txt"
  if grep -q 'shared object' "$audit_raw_dir/artifact_file.txt"; then
    audit_artifact_is_shared_object=true
  fi
else
  : > "$audit_raw_dir/artifact_file.txt"
  : > "$audit_raw_dir/artifact_elf_header.txt"
  : > "$audit_raw_dir/artifact_dynamic_symbols.txt"
fi

nm --version | sed -n '1p' > "$audit_raw_dir/oracle_version.txt"

if [ "$audit_build_exit" -ne 0 ]; then
  audit_classification='SETUP ERROR'
  audit_reason='isolated_build_failed'
elif [ "$audit_artifact_present" != true ]; then
  audit_classification='SETUP ERROR'
  audit_reason='declared_shared_library_artifact_missing'
elif [ "$audit_main_count" -eq 0 ] && [ "$audit_parser_count" -eq 0 ] && [ "$audit_artifact_is_shared_object" = true ]; then
  audit_classification='PROFILE NOT APPLICABLE'
  audit_reason='no_production_elf_input_symbol_output_command'
else
  audit_classification='REQUIRES_MANUAL_REVIEW'
  audit_reason='profile_b_capability_signal_detected'
fi

{
  printf 'classification=%s\n' "$audit_classification"
  printf 'reason=%s\n' "$audit_reason"
  printf 'isolated_build_exit=%s\n' "$audit_build_exit"
  printf 'artifact_present=%s\n' "$audit_artifact_present"
  printf 'artifact_is_shared_object=%s\n' "$audit_artifact_is_shared_object"
  printf 'production_main_matches=%s\n' "$audit_main_count"
  printf 'elf_parser_signal_matches=%s\n' "$audit_parser_count"
  printf 'target_command_available=false\n'
} > "$audit_raw_dir/profile_b_applicability.env"

cat "$audit_raw_dir/profile_b_applicability.env"
