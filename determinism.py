import subprocess
import hashlib
import time

RUNS = 100
args = ["./build/redstone", "demo_repro3.toml"]

sha = None

runs = []



for i in range(RUNS):
    result = subprocess.run(args, stdout=subprocess.PIPE)
    result.check_returncode()

    print("finished run!")

    runs.append(result.stdout)

    with open(f"runs/run{i}.txt", "wb") as f:
        _ = f.write(result.stdout)

    iter_sha = hashlib.sha256(result.stdout).digest()
    if sha is None:
        sha = iter_sha
    if sha != iter_sha:
        print(sha, iter_sha)
        print("run had different sha")
    time.sleep(1)
