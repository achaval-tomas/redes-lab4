#!/bin/python3
import subprocess
import re

def export_vec(gen_interval, module, name):
    filename=f"./results/{module}_{name}_{gen_interval}.csv"
    subprocess.run(f'opp_scavetool x ./results/General-#0.vec -F CSV-S -o {filename} -f "module=~{module} AND name=~{name}"', shell=True)
    subprocess.run(f"sed -i '1d' {filename}", shell=True)

with open("./results/General-#0.vci", "r") as vci_file:
    data = vci_file.read()
    print(data)

    gen_interval_match = re.search(r"exponential\((.*)\)", data)
    gen_interval = gen_interval_match.group(1)

    vec_matches = re.findall(r"vector \d+ ([A-Za-z.\[\]\d]+) (.*) ETV", data)
    for vec_match in vec_matches:
        module = vec_match[0]
        name = vec_match[1]
        export_vec(gen_interval, module, name)
