#!/bin/python3
import matplotlib
import matplotlib.axes
import matplotlib.lines
import matplotlib.pyplot as plt
import pathlib

vectors = [
    "Network.node[0].lnk[0]_BufferSize_1.0",
    "Network.node[1].lnk[0]_BufferSize_1.0",
    "Network.node[2].lnk[0]_BufferSize_1.0",
    "Network.node[6].lnk[0]_BufferSize_1.0",
    "Network.node[7].lnk[0]_BufferSize_1.0",
]

x_label = "Time (seconds)"
y_label = "Buffer Size (packets)"


def plot_vector(ax: matplotlib.axes.Axes, file: str) -> matplotlib.lines.Line2D:
    x_axis = []
    y_axis = []
    with open(file, "r") as f:
        for line in f:
            [x, y] = line.split(",")
            x_axis.append(float(x))
            y_axis.append(float(y))
    label = pathlib.Path(file).stem.replace("_", " ")
    return ax.plot(x_axis, y_axis, label=label)[0]


def enable_line_toggling(fig, lines, legend):
    pickradius = 5
    map_legend_to_ax = {}

    for legend_line, ax_line in zip(legend.get_lines(), lines):
        legend_line.set_picker(pickradius)
        map_legend_to_ax[legend_line] = ax_line

    def on_pick(event):
        legend_line = event.artist
        if legend_line not in map_legend_to_ax:
            return

        ax_line = map_legend_to_ax[legend_line]
        visible = not ax_line.get_visible()
        ax_line.set_visible(visible)
        legend_line.set_alpha(1.0 if visible else 0.2)
        fig.canvas.draw()

    fig.canvas.mpl_connect('pick_event', on_pick)
    # legend.set_draggable(True)


fig, ax = plt.subplots()
ax: matplotlib.axes.Axes
lines = [plot_vector(ax, "./results/" + vector + ".csv") for vector in vectors]
ax.set_ylim(bottom=0)
ax.set_xlim(left=0)
ax.grid(True)
legend = ax.legend()
# ax.axhline(0, color='gray')
# ax.axvline(0, color='gray')
ax.set_xlabel(x_label)
ax.set_ylabel(y_label)

enable_line_toggling(fig, lines, legend)

plt.show()
# plt.savefig("test.svg")
