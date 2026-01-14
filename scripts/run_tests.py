#!/usr/bin/env python3
import argparse
import json
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
    args = parser.parse_args()

    tests_dir = Path(args.tests_dir)
    if not tests_dir.is_dir():
        print(f"Error: tests dir not found: {tests_dir}", file=sys.stderr)
        return 2

    test_files = sorted(tests_dir.glob(args.pattern))
    if not test_files:
        print(f"No tests found in {tests_dir} with pattern {args.pattern}", file=sys.stderr)
        return 2

    results = []
    for test_path in test_files:
        results.append(run_test(args.binary, test_path, args.explain))

    failed = [r for r in results if r["code"] != 0]

    print(f"Total: {len(results)} | OK: {len(results) - len(failed)} | Failed: {len(failed)}")
    for r in results:
        status = "OK" if r["code"] == 0 else "FAIL"
        print(f"{status} {r['test']}")
        if r["stdout"]:
            print("  stdout:")
            for line in r["stdout"].splitlines():
                print(f"    {line}")
        if r["stderr"]:
            print("  stderr:")
            for line in r["stderr"].splitlines():
                print(f"    {line}")

    if args.json_path:
        with open(args.json_path, "w", encoding="utf-8") as f:
            json.dump(results, f, indent=2)

    return 0 if not failed else 1


if __name__ == "__main__":
    raise SystemExit(main())
