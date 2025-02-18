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
    # Execute evaluate and save the poses of time
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

    outfile_name = "beam_length_analysis_{:d}.txt".format(int(time.time()))
    writer = csv.writer(open(outfile_name, 'w'))
    with Pool(processes=args.pool_size) as pool:
        pool_args = [(args.params, f, args.library_path, args.trials, args.verbose) for f in args.argos_files]
        all_costs = pool.map(evaluate, pool_args)

        for (argos_file, costs) in zip(args.argos_files, all_costs):
            writer.writerow([argos_file] + costs)


def plot_func(args):
    import matplotlib.pyplot as plt
    from myboxplot import my_boxplot
    style_dir = os.path.dirname(os.path.realpath(__file__))
    style = os.path.join(style_dir, "mpl.style")
    plt.style.use(style)

    reader = csv.reader(open(args.beam_length_output, 'r'), delimiter=',')
    costs = []
    proportions_to_max = []
    for row in reader:
        m = re.search("(\d+)_percent", row[0])
        p = float(m.groups()[0])
        proportions_to_max.append(p)
        costs_for_length = [float(i) for i in row[1:]]
        costs.append(costs_for_length)

    proportions_to_max = np.array(proportions_to_max)
    costs = np.array(costs)
    sorted_indeces = np.argsort(proportions_to_max)
    proportions_to_max = proportions_to_max[sorted_indeces]
    costs = costs[sorted_indeces]

    fig, ax = plt.subplots()
    my_boxplot(ax, proportions_to_max, costs, width=0.5)
    plt.xlabel("Sensor Range, Percentage of Maximum")
    plt.ylabel("Cost")
    plt.show()


def main():
    parser = argparse.ArgumentParser("Evaluate cost over the beam length.")
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
    plot.add_argument("beam_length_output", help='file output by the evaluate subcommand')
    plot.set_defaults(func=plot_func)

    args = parser.parse_args()
    args.func(args)

    return 0


if __name__ == '__main__':
    sys.exit(main())
