import subprocess
import sys
import os

def test_hello_runs_without_error():
    result = subprocess.run(
        [sys.executable, os.path.join(os.path.dirname(__file__), "hello_world.py")],
        capture_output=True,
        text=True
    )
    assert result.returncode == 0, f"Script failed with: {result.stderr}"
    assert "Hello from Gas Town!" in result.stdout, f"Unexpected output: {result.stdout}"
    print(f"Output: {result.stdout.strip()}")

if __name__ == "__main__":
    test_hello_runs_without_error()
