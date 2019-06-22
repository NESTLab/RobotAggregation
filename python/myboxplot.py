import matplotlib.pyplot as plt
import numpy as np


def my_boxplot(ax, positions, values, width=None, color=None, label=None):
    """Custom box plot to work around some of matplotlib's quirks.
    Parameters
    ----------
    ax : matplotlib axis
        Target axis.
    positions : (M,) ndarray of float
        Where to positions boxes on the x-axis.
    values : (M, N) ndarray of float
        The percentiles of each row of ``values`` is box-plotted.
    width : float
        Width of the boxes.
    color : str
        Matplotlib color code.
    """
    if width is None:
        width = 1
    if color is None:
        color = 'k'

    positions = [int(p) for p in positions]
    x = np.column_stack((positions, positions))
    p25, p75 = np.percentile(values, [25, 75], axis=1)
    whisker_lower = np.column_stack((np.min(values, axis=1), p25))
    whisker_upper = np.column_stack((p75, np.max(values, axis=1)))
    plt.plot(x.T, whisker_lower.T, '%s-_' % color)
    plt.plot(x.T, whisker_upper.T, '%s-_' % color)

    zero_mid_percentile, = np.where(p25 == p75)
    w = width / 2.
    for ix in zero_mid_percentile:
        pos = positions[ix]
        med = np.median(values[ix])
        plt.plot([pos - w, pos + w], [med, med], linewidth=6)

    bp = ax.boxplot(values.T, positions=positions, sym='', widths=width, whis=0)
    plt.setp(bp['boxes'], linewidth=1)
    plt.setp(bp['medians'], linewidth=2)
    plt.setp(bp['caps'], visible=False)
    plt.setp(bp['fliers'], visible=False)

    ax.plot(positions, np.median(values, axis=1), linewidth=2, label=label)
    ax.grid(True)
    plt.setp(ax.get_xticklabels()[1::2], visible=False)
