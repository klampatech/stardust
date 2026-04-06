"""Core functionality for Vault Sentry."""

import json
import os
import time
from pathlib import Path
from typing import Any


class VaultSentry:
    """Main sentry class for monitoring and state reporting."""

    def __init__(self, state_file: Path | None = None):
        """Initialize the sentry.

        Args:
            state_file: Optional path to state file. Defaults to ~/.vault_sentry/state.json
        """
        if state_file is None:
            state_file = Path.home() / ".vault_sentry" / "state.json"
        self.state_file = state_file

    def get_state(self) -> dict[str, Any]:
        """Get the current state.

        Returns:
            Dictionary containing current state.
        """
        if self.state_file.exists():
            try:
                with open(self.state_file) as f:
                    return json.load(f)
            except (json.JSONDecodeError, IOError):
                pass

        return {
            "status": "unknown",
            "uptime_seconds": 0,
            "last_report": None,
        }

    def report(self) -> dict[str, Any]:
        """Generate a report of current state.

        Returns:
            Dictionary containing the report data.
        """
        state = self.get_state()
        report = {
            "status": state.get("status", "unknown"),
            "uptime_seconds": state.get("uptime_seconds", 0),
            "last_report": state.get("last_report"),
            "timestamp": time.time(),
        }
        return report

    def watch(self, interval: float = 1.0) -> None:
        """Run in daemon mode, reporting state at regular intervals.

        Args:
            interval: Seconds between state checks. Defaults to 1.0.
        """
        state = self.get_state()
        start_time = time.time()
        state["status"] = "running"
        state["start_time"] = start_time

        self._ensure_state_dir()
        self._save_state(state)

        while True:
            current_time = time.time()
            elapsed = current_time - start_time
            state["uptime_seconds"] = elapsed
            state["last_report"] = current_time
            self._save_state(state)
            time.sleep(interval)

    def _ensure_state_dir(self) -> None:
        """Ensure the state directory exists."""
        self.state_file.parent.mkdir(parents=True, exist_ok=True)

    def _save_state(self, state: dict[str, Any]) -> None:
        """Save state to file.

        Args:
            state: State dictionary to save.
        """
        self._ensure_state_dir()
        try:
            with open(self.state_file, "w") as f:
                json.dump(state, f)
        except IOError:
            pass