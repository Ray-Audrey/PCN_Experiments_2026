import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results.csv")
df["Bandwidth"] = df["Bandwidth"].str.replace("Mbps", "").astype(float)

for m in ["LossPct", "AvgDelayMs", "ThroughputMbps", "TCPRetransRatePct"]:
    df.pivot(index="Bandwidth", columns="Flow", values=m).plot(kind="bar")
    plt.title("TCP vs UDP: " + m)
    plt.ylabel(m)
    plt.tight_layout()
    plt.show()