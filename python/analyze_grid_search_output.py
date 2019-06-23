#!/usr/bin/env python

import argparse
import csv
import os

import numpy as np
from scipy import stats


def main():
    parser = argparse.ArgumentParser("Visualize a set of output files from grid search")
    parser.add_argument("merged_grid_search_outputs")
    parser.add_argument("--outfile")
    parser.add_argument("--resolution", type=int, default=7)
    parser.add_argument("--best-n", type=int, default=10)
    parser.add_argument("--ignore-known-controllers", action="store_true")
    parser.add_argument("--plot", action="store_true")
    parser.add_argument("--viz", action="store_true")
    parser.add_argument("--save", action="store_true")
    parser.add_argument("--exclude-one-class", action="store_true")

    args = parser.parse_args()

    if args.plot or args.viz:
        style_dir = os.path.dirname(os.path.realpath(__file__))
        style = os.path.join(style_dir, "mpl.style")
        import matplotlib.pyplot as plt
        plt.style.use(style)

    f = np.genfromtxt(args.merged_grid_search_outputs, skip_header=True)

    env_start_idx = 7
    all_costs = f[:, env_start_idx:]
    if args.exclude_one_class:
        # skip the first 8 environments, they are for the 1-class scenarios
        all_costs = all_costs[:, 8:]
    mean_costs = np.mean(all_costs, axis=1)
    std_costs = np.std(all_costs, axis=1)
    params = f[:, 1:7]

    if args.outfile:
        writer = csv.writer(open(args.outfile, 'w'), delimiter=',')
        for ss_diff_index, (p, c, s) in enumerate(zip(params, mean_costs, std_costs)):
            writer.writerow([ss_diff_index, p, c, s])

    if args.viz or args.save:
        axes_titles = [r'$v_{l_0}$', r'$v_{r_0}$', r'$v_{l_1}$', r'$v_{r_1}$', r'$v_{l_2}$', r'$v_{r_2}$']
        for x_param in range(6):
            for y_param in range(x_param + 1, 6):

                plt.figure()
                plt.xlabel(axes_titles[x_param], fontsize=32)
                plt.ylabel(axes_titles[y_param], fontsize=32, rotation=0)
                labels = ['-1.0', '', '', '', '', '', '1.0']
                plt.xticks(np.arange(7), labels, fontsize=24)
                plt.yticks(np.arange(7), labels, fontsize=24)

                shape = tuple([args.resolution] * 6)
                cost_image = np.ones((args.resolution, args.resolution)) * 1e24
                for parameter_idx, cost in enumerate(mean_costs):
                    indeces = np.unravel_index(parameter_idx, shape)
                    row = indeces[x_param]
                    col = indeces[y_param]
                    if cost < cost_image[row, col]:
                        cost_image[row, col] = cost

                plt.imshow(cost_image, cmap='Reds')
                if args.save:
                    plt.savefig("{:d}_{:d}_grid_img.png".format(x_param, y_param))

    # Sorting messes up plotting so we have to do this after
    sorted_cost_indeces = mean_costs.argsort(axis=0)
    mean_costs.sort()
    params = params[sorted_cost_indeces]
    std_costs = std_costs[sorted_cost_indeces]
    all_costs = all_costs[sorted_cost_indeces]
    print("Best Params, Index, Cost")
    print("{} {:0.0f} {:0.0f}".format(params[0], mean_costs[0], std_costs[0]))
    print("=" * 85)

    print("Good params")
    unknown_controllers = 0
    for ss_diff_index, p in enumerate(params[:args.best_n]):
        # check if this matches the "patterns" of known segregating controllers
        # this never prints anything because turns out they all can be described this way
        if args.ignore_known_controllers:
            if p[0] > p[1]:
                if p[4] > p[5]:
                    if p[3] >= p[2]:
                        # left-hand circles segregating
                        continue
                    else:
                        # slow clustering segregation?
                        continue
            elif p[0] < p[1]:
                if p[5] > p[4]:
                    if p[2] >= p[3]:
                        # right-hand circles segregating
                        continue
                    else:
                        # slow clustering segregation?
                        continue
        unknown_controllers += 1
        print(p, "{:d}th {:0.0f} {:0.0f}".format(ss_diff_index, mean_costs[ss_diff_index], std_costs[ss_diff_index]))

    best_cost = all_costs[0]
    inconclusive = True
    for ss_diff_index, costs in enumerate(all_costs):
        t_value, p_value = stats.ttest_ind(best_cost, costs, equal_var=False)
        if np.allclose(params[ss_diff_index], [1, -2 / 3, 1 / 3, 1, 1, 0]):
            print("P value of old params: ", ss_diff_index, p_value, mean_costs[ss_diff_index],
                  std_costs[ss_diff_index], mean_costs[0], std_costs[0],
                  params[ss_diff_index])
            break
        if p_value < 0.05 and inconclusive:
            inconclusive = False
            print("top {} params are not statistically significantly different".format(ss_diff_index))
            break
    if inconclusive:
        print("cannot conclude that the worst params are not identical mean to the best params...")

    if args.plot:
        plt.figure()
        every_n = 50
        c = mean_costs[::every_n]
        N = len(c)
        ss_diff_cost = mean_costs[ss_diff_index]
        markers, caps, bars = plt.errorbar(np.arange(N), c, yerr=std_costs[::every_n], xerr=None, fmt='.',
                                           ecolor='black', zorder=1,
                                           capsize=2, capthick=2)
        # markers.set_zorder(-1)
        [bar.set_alpha(0.05) for bar in bars]
        [cap.set_alpha(0.2) for cap in caps]

        plt.plot([0, N], [ss_diff_cost, ss_diff_cost], c='r', zorder=2)
        plt.plot([0, N], [mean_costs[0] + std_costs[0], mean_costs[0] + std_costs[0]], c='#23FF00', zorder=2)
        plt.plot([0, N], [mean_costs[0] + 2 * std_costs[0], mean_costs[0] + 2 * std_costs[0]], c='#17AA00', zorder=2)
        plt.plot([0, N], [mean_costs[0], mean_costs[0]], c='k', zorder=1)

        plt.xlabel("Parameters from Grid Search (sorted)")
        plt.ylabel("Cost")
        plt.gca().set_xticklabels([])

    if args.plot or args.viz:
        plt.show()


if __name__ == "__main__":
    main()
