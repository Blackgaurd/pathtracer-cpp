import matplotlib.pyplot as plt

spp = [1, 5, 10, 50, 100, 500, 1000, 5000, 10000]
seconds = [0.05, 0.27, 0.52, 2.49, 4.87, 21.61, 42.03, 205.49, 410.02]

plt.plot(spp, seconds, "o-")
plt.title("CPU rendering time vs. samples per pixel")
plt.ylabel("Seconds")
plt.xlabel("Samples per pixel")
plt.savefig("benchmark/spp/graph.png")

# combine the images to a gif
# how to loop gif in markdown: https://stackoverflow.com/a/18473154
import imageio

allimages = [f"benchmark/spp/spp{i}.png" for i in spp]
imageio.mimsave(
    "benchmark/spp/spp.gif", [imageio.imread(fn) for fn in allimages], duration=200
)
