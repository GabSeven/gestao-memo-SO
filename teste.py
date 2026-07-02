import subprocess

metodos = ["FIRST_FIT", "WORST_FIT", "BEST_FIT", "BUDDY"]

for i in range(1, 12 + 1):
    for m in metodos:
        subprocess.run(["./a.out",m, f"exemplos_entrada/entrada{i:03d}.txt"])
        # print(f"./a.out {m} exemplos_entrada/entrada{i:03d}.txt")

saidaM = ["best", "buddy", "worst", "first"]

for i in range(1, 12 + 1):
    for m in saidaM:
        print(i, m)
        subprocess.run(["diff",
        f"out/log_entrada{i:03d}_{m}.txt",
        f"exemplos_saida/log_entrada{i:03d}_{m}.txt"
        ])
