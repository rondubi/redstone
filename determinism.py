import subprocess
import hashlib

RUNS = 100
args = ["./build/redstone", "sim.toml"]

sha = None

for _ in range(RUNS):
    result = subprocess.run(args, stdout=subprocess.PIPE)
    result.check_returncode()

    print("finished run!")

    iter_sha = hashlib.sha256(result.stdout)
    if sha is None:
        sha = iter_sha
    if sha != iter_sha:
        print("run had different sha")