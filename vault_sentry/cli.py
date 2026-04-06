"""CLI entry point for vault-sentry."""

import argparse
import json
import sys
from typing import TextIO

from vault_sentry.core import VaultSentry


def main() -> int:
    """Main entry point for vault-sentry CLI.

    Returns:
        Exit code (0 for success, non-zero for error).
    """
    parser = argparse.ArgumentParser(
        prog="vault-sentry",
        description="Vault Sentry - Monitoring and reporting tool",
    )

    parser.add_argument(
        "--report",
        action="store_true",
        help="Print current state to stdout",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    parser.add_argument(
        "--watch",
        action="store_true",
        help="Run in daemon mode",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=1.0,
        help="Seconds between reports in watch mode (default: 1.0)",
    )
    parser.add_argument(
        "--state-file",
        type=str,
        help="Path to state file",
    )

    args = parser.parse_args()

    if not args.report and not args.watch:
        parser.print_help()
        return 1

    sentry = VaultSentry()

    if args.report:
        return report_cmd(sentry, args.format)

    if args.watch:
        watch_cmd(sentry, args.interval)
        return 0

    return 1


def report_cmd(sentry: VaultSentry, output_format: str) -> int:
    """Handle the --report command.

    Args:
        sentry: VaultSentry instance.
        output_format: Either 'text' or 'json'.

    Returns:
        Exit code.
    """
    report_data = sentry.report()

    if output_format == "json":
        json.dump(report_data, sys.stdout)
        print()
        return 0

    _print_text_report(report_data)
    return 0


def _print_text_report(data: dict) -> None:
    """Print report in text format.

    Args:
        data: Report data dictionary.
    """
    print("Vault Sentry Report")
    print("=" * 40)
    print(f"Status: {data.get('status', 'unknown')}")
    print(f"Uptime: {data.get('uptime_seconds', 0):.1f} seconds")
    if data.get("last_report"):
        print(f"Last Report: {data['last_report']}")
    print()


def watch_cmd(sentry: VaultSentry, interval: float) -> None:
    """Handle the --watch command (runs indefinitely).

    Args:
        sentry: VaultSentry instance.
        interval: Seconds between reports.
    """
    try:
        sentry.watch(interval)
    except KeyboardInterrupt:
        print("\nShutting down...")
        sys.exit(0)


if __name__ == "__main__":
    sys.exit(main())