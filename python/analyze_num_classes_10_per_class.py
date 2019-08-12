#!/usr/bin/env python3

import re
import argparse
import csv
import os
import subprocess
import sys
import time

from multiprocessing import Pool
import numpy as np



def evaluate(args):
    params_file, argos_file, library_path, trials, verbose = args
    cmd = ["./build/bin/evaluate", "-t", str(trials), argos_file, library_path, params_file]
    cmd_str = " ".join(cmd)
    output = subprocess.run(cmd_str, stdout=subprocess.PIPE, shell=True)
    if output.returncode != 0:
        print("subprocess failed:")
        print(" ".join(cmd))
        output = output.stdout.decode("UTF-8")
        print(output)
        return -1

    costs = []
    output = output.stdout.decode("UTF-8").split("\n")[8:-1]
    for line in output:
        try:
            cost = float(line.split(" ")[-1])
        except ValueError:
            continue
        costs.append(cost)
    if verbose:
        print(cmd_str, costs)
    return costs


def eval_func(args):
    params = np.genfromtxt(args.params)[1:]
    if args.verbose:
        print(params)

    outfile_name = "n_classes_analysis_{:d}.txt".format(int(time.time()))
    writer = csv.writer(open(outfile_name, 'w'))
    with Pool(processes=args.pool_size) as pool:
        pool_args = [(args.params, f, args.library_path, args.trials, args.verbose) for f in args.argos_files]
        all_costs = pool.map(evaluate, pool_args)

        for (argos_file, costs) in zip(args.argos_files, all_costs):
            writer.writerow([argos_file] + all_costs)


def plot_func(args):
    import matplotlib.pyplot as plt
    from myboxplot import my_boxplot
    style_dir = os.path.dirname(os.path.realpath(__file__))
    style = os.path.join(style_dir, "mpl.style")
    plt.style.use(style)

    reader = csv.reader(open(args.n_classes_output, 'r'), delimiter=',')
    costs = []
    n_classes = []
    max_costs = []
    # This must match the default integer value in evaluate.cpp
    T = 180
    for row in reader:
        m = re.search("(\d+)_class", row[0])
        n_class = float(m.groups()[0])
        n_classes.append(n_class)
        costs_for_n_class = [float(i) for i in row[1:]]
        costs.append(costs_for_n_class)
        max_costs.append(-1.0/10)

    # Sort both lists based on n_classes
    n_classes = np.array(n_classes)
    costs = np.array(costs)
    max_costs = np.array(max_costs)
    sorted_indeces = np.argsort(n_classes)
    n_classes = n_classes[sorted_indeces]
    costs = costs[sorted_indeces]
    max_costs = max_costs[sorted_indeces]

    fig, ax = plt.subplots()
    ax.plot(n_classes, max_costs, linestyle='--', c='r', linewidth=4, label='worst case')
    my_boxplot(ax, n_classes, costs, width=0.5)
    plt.xlabel("Number of Classes")
    plt.ylabel("Cost")
    plt.legend()
    plt.show()


def main():
    parser = argparse.ArgumentParser("Evaluate cost over argos files with varying number of classes.")
    subparsers = parser.add_subparsers()
    evaluate = subparsers.add_parser('evaluate', help="run the simulations")
    evaluate.add_argument("argos_files", help="the argos files you want to run evaluate with", nargs="+")
    evaluate.add_argument("library_path", help="the path to the loop function *.so library to use")
    evaluate.add_argument("params", help="params file to evaluate")
    evaluate.add_argument("--pool-size", "-p", help="number of worker subprocesses to spawn", type=int, required=True)
    evaluate.add_argument("--trials", '-t', help="number of trials per argos configuration", type=int, default=100)
    evaluate.add_argument("--verbose", "-v", help="print more shit", action="store_true")
    evaluate.set_defaults(func=eval_func)
    plot = subparsers.add_parser('plot', help="plot the output of \'evaluate\'")
    plot.add_argument("n_classes_output", help='file output by the evaluate subcommand')
    plot.set_defaults(func=plot_func)

    args = parser.parse_args()
    args.func(args)

    return 0


if __name__ == '__main__':
    sys.exit(main())
