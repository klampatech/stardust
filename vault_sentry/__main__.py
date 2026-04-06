"""Allow running as: python -m vault_sentry"""

from vault_sentry.cli import main

if __name__ == "__main__":
    raise SystemExit(main())