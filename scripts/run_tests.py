#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


def run_test(binary, test_path, explain):
    cmd = [binary, str(test_path)]
    if explain:
        cmd.append("--explain")
    proc = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    return {
        "test": str(test_path),
        "code": proc.returncode,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }

EXPECTED_RE = re.compile(r"\b([A-Z])\s*=\s*(true|false|ambiguous|ambigous)\b", re.IGNORECASE)


def parse_expected(test_path):
    expected = {}
    conflicts = {}
    lines = test_path.read_text(encoding="utf-8").splitlines()
    for line in lines:
        stripped = line.strip()
        if not stripped.startswith("#"):
            continue
        if "Expected:" not in stripped:
            continue
        for match in EXPECTED_RE.finditer(stripped):
            var = match.group(1)
            val = match.group(2).lower()
            if val == "ambigous":
                val = "ambiguous"
            if var in expected and expected[var] != val:
                conflicts[var] = (expected[var], val)
            expected[var] = val
    return expected, conflicts


def parse_actual(stdout):
    actual = {}
    for line in stdout.splitlines():
        for match in EXPECTED_RE.finditer(line):
            var = match.group(1)
            val = match.group(2).lower()
            if val == "ambigous":
                val = "ambiguous"
            actual[var] = val
    return actual


def evaluate_result(expected, conflicts, actual):
    if conflicts:
        return False, f"conflicting expected values: {conflicts}"
    if not expected:
        return False, "no parsable expected values"
    missing = [var for var in expected if var not in actual]
    mismatched = [
        var for var in expected if var in actual and actual[var] != expected[var]
    ]
    if missing:
        return False, f"missing outputs: {', '.join(missing)}"
    if mismatched:
        details = ", ".join(f"{v}={actual[v]} expected {expected[v]}" for v in mismatched)
        return False, f"mismatched outputs: {details}"
    return True, ""


def main():
    parser = argparse.ArgumentParser(
        description="Run expert system binary against all tests and summarize results."
    )
    parser.add_argument(
        "--binary",
        default="./expert",
        help="Path to the expert binary (default: ./expert)",
    )
    parser.add_argument(
        "--tests-dir",
        default="tests",
        help="Directory containing .txt tests (default: tests)",
    )
    parser.add_argument(
        "--pattern",
        default="*.txt",
        help="Glob pattern for tests (default: *.txt)",
    )
    parser.add_argument(
        "--explain",
        action="store_true",
        help="Pass --explain to the binary",
    )
    parser.add_argument(
        "--json",
        dest="json_path",
        help="Write full results to a JSON file",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print stdout/stderr for each test",
    )
    parser.add_argument(
        "--only-fail",
        action="store_true",
        help="Only print failing tests",
    )
    parser.add_argument(
        "--include-legacy",
        action="store_true",
        help="Include tests under legacy folders",
    )
    args = parser.parse_args()

    tests_dir = Path(args.tests_dir)
    if not tests_dir.is_dir():
        print(f"Error: tests dir not found: {tests_dir}", file=sys.stderr)
        return 2

    test_files = sorted(
        p for p in tests_dir.rglob(args.pattern)
        if p.is_file() and (args.include_legacy or "legacy" not in p.parts)
    )
    if not test_files:
        print(f"No tests found in {tests_dir} with pattern {args.pattern}", file=sys.stderr)
        return 2

    results = []
    for test_path in test_files:
        result = run_test(args.binary, test_path, args.explain)
        expected, conflicts = parse_expected(test_path)
        actual = parse_actual(result["stdout"])
        ok = result["code"] == 0
        detail = ""
        if ok:
            ok, detail = evaluate_result(expected, conflicts, actual)
        else:
            detail = result["stderr"] or "non-zero exit code"
        result["expected"] = expected
        result["actual"] = actual
        result["ok"] = ok
        result["detail"] = detail
        results.append(result)

    failed = [r for r in results if not r["ok"]]

    for r in results:
        status = "ok" if r["ok"] else "ko"
        if args.only_fail and r["ok"]:
            continue
        print(f"{Path(r['test']).name} : {status}")
        if not r["ok"] and r["detail"]:
            print(f"  {r['detail']}")
        if args.verbose and r["stdout"]:
            print("  stdout:")
            for line in r["stdout"].splitlines():
                print(f"    {line}")
        if args.verbose and r["stderr"]:
            print("  stderr:")
            for line in r["stderr"].splitlines():
                print(f"    {line}")

    if args.json_path:
        with open(args.json_path, "w", encoding="utf-8") as f:
            json.dump(results, f, indent=2)

    print(f"Total: {len(results)} | OK: {len(results) - len(failed)} | Failed: {len(failed)}")
    return 0 if not failed else 1


if __name__ == "__main__":
    raise SystemExit(main())
