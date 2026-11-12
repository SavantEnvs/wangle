#!/usr/bin/env bash
#
# mayhem/test.sh — run wangle's own gtest binaries (built by mayhem/build.sh, normal flags) and report
# their counts as CTRF. Behavior is asserted by the suites themselves (frame contents, ticket seeds,
# pipeline ordering); a run that reports zero tests fails.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

passed=0; failed=0; skipped=0; ran=0
for t in CodecTest PipelineTest TLSCredProcessorTest; do
  bin="$SRC/mayhem/build-tests/bin/$t"
  if [ ! -x "$bin" ]; then
    echo "missing test binary $bin (mayhem/build.sh should have built it)" >&2
    emit_ctrf "wangle-gtest" 0 1 0
    exit 1
  fi
  out=$("$bin" 2>&1); rc=$?
  printf '%s\n' "$out" | tail -n 15
  n_run=$(printf '%s\n' "$out" | sed -n 's/^\[==========\] \([0-9]\+\) tests\? from .* ran\..*/\1/p' | tail -n1)
  n_pass=$(printf '%s\n' "$out" | sed -n 's/^\[  PASSED  \] \([0-9]\+\) tests\?\./\1/p' | tail -n1)
  n_skip=$(printf '%s\n' "$out" | sed -n 's/^\[  SKIPPED \] \([0-9]\+\) tests\?, listed below:/\1/p' | tail -n1)
  n_fail=$(printf '%s\n' "$out" | sed -n 's/^\[  FAILED  \] \([0-9]\+\) tests\?, listed below:/\1/p' | tail -n1)
  n_run=${n_run:-0}; n_pass=${n_pass:-0}; n_skip=${n_skip:-0}; n_fail=${n_fail:-0}
  if [ "$n_run" -eq 0 ]; then
    echo "$t reported no tests" >&2; n_fail=$(( n_fail + 1 ))
  elif [ "$rc" -ne 0 ] && [ "$n_fail" -eq 0 ]; then
    n_fail=1
  fi
  passed=$(( passed + n_pass )); failed=$(( failed + n_fail )); skipped=$(( skipped + n_skip ))
  ran=$(( ran + n_run ))
done
[ "$ran" -gt 0 ] || failed=$(( failed + 1 ))
emit_ctrf "wangle-gtest" "$passed" "$failed" "$skipped"
