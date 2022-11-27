#!/usr/bin/env python3
import matplotlib.pyplot as plt
import json
import sys

fig, axs = plt.subplots(3, 1, dpi=300, figsize=(10, 10))
fig.tight_layout()

results = {}
data = json.loads(sys.stdin.read())
for bench in data['benchmarks']:
    if '/' not in bench['name']:
        continue
    policy = bench['name'].split('<')[1].split(',')[0]
    dataset = bench['name'].split('<')[1].split(',')[1].split('>')[0][1:]
    sz = bench['name'].split('/')[1]
    cpu = bench['cpu_time']
    hitrate = bench['hit_rate']
    if dataset not in results:
        results[dataset] = {}
    if policy not in results[dataset]:
        results[dataset][policy] = ([], [])
    results[dataset][policy][0].append(cpu)
    results[dataset][policy][1].append(sz)

for (i, dataset) in zip(range(len(results)), results.values()):
    for (j, (policy, values)) in zip(range(len(dataset.values())), dataset.items()):
        axs[i].plot(values[1],
                    values[0],
                    color='C{0}'.format(j),
                    label=policy)

for (subplot, title) in zip(axs, results.keys()):
    subplot.title.set_text(title)
    subplot.legend()
plt.savefig('benchmarks.jpg')
