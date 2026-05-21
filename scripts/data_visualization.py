# ─────────────────────────────────────────
#  EchoNav — scripts/data_visualization.py
#  Reads the user testing CSV data and
#  generates charts for the paper:
#
#  1. Heart rate before vs during testing
#  2. Task completion rates per participant
#  3. Detection accuracy per object class
#
#  Run after user testing is complete:
#  python data_visualization.py
#
#  Requires: matplotlib, pandas, numpy
#  Install: pip install matplotlib pandas numpy
# ─────────────────────────────────────────

import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# ── Paths ─────────────────────────────────
RESULTS_CSV   = "../ml/evaluation/test_results.csv"
OUTPUT_DIR    = "../ml/evaluation/"

# ── Colours matching RED/YELLOW/GREEN ─────
COLOR_HIGH   = "#e74c3c"   # RED
COLOR_MEDIUM = "#f39c12"   # YELLOW
COLOR_LOW    = "#2ecc71"   # GREEN
COLOR_BASE   = "#3498db"   # blue for neutral bars


# ─────────────────────────────────────────
#  Chart 1: Heart rate before vs during
#  Shows that EchoNav reduces stress
#  compared to navigating without it
# ─────────────────────────────────────────
def plot_heart_rate(df):
    participants = df["participant"].unique()

    before = df.groupby("participant")["bpm_before"].mean()
    during = df.groupby("participant")["bpm_during"].mean()

    x = np.arange(len(participants))
    width = 0.35

    fig, ax = plt.subplots(figsize=(8, 5))

    bars1 = ax.bar(x - width/2, before, width,
                   label="Before (no EchoNav)",
                   color=COLOR_HIGH, alpha=0.85)
    bars2 = ax.bar(x + width/2, during, width,
                   label="During (with EchoNav)",
                   color=COLOR_LOW, alpha=0.85)

    ax.set_title("Heart Rate: Before vs During EchoNav Navigation",
                 fontsize=13, fontweight="bold")
    ax.set_xlabel("Participant")
    ax.set_ylabel("Average BPM")
    ax.set_xticks(x)
    ax.set_xticklabels([f"P{p}" for p in participants])
    ax.legend()
    ax.set_ylim(50, 120)

    # Add value labels on bars
    for bar in bars1:
        ax.text(bar.get_x() + bar.get_width()/2,
                bar.get_height() + 1,
                f"{bar.get_height():.0f}",
                ha="center", va="bottom", fontsize=9)
    for bar in bars2:
        ax.text(bar.get_x() + bar.get_width()/2,
                bar.get_height() + 1,
                f"{bar.get_height():.0f}",
                ha="center", va="bottom", fontsize=9)

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, "heart_rate_comparison.png")
    plt.savefig(path, dpi=150)
    plt.close()
    print(f"[SAVED] {path}")


# ─────────────────────────────────────────
#  Chart 2: Task completion rates
#  Shows how well each participant
#  completed the 3 navigation tasks
# ─────────────────────────────────────────
def plot_task_completion(df):
    tasks = ["hallway_clear", "door_found", "person_detected"]
    task_labels = ["Clear hallway", "Find door", "Detect person"]

    completion = [df[task].mean() * 100 for task in tasks]

    fig, ax = plt.subplots(figsize=(7, 5))

    bars = ax.bar(task_labels, completion,
                  color=[COLOR_LOW, COLOR_MEDIUM, COLOR_BASE],
                  alpha=0.85, width=0.5)

    ax.set_title("Task Completion Rate Across All Participants",
                 fontsize=13, fontweight="bold")
    ax.set_ylabel("Completion Rate (%)")
    ax.set_ylim(0, 110)

    for bar in bars:
        ax.text(bar.get_x() + bar.get_width()/2,
                bar.get_height() + 1.5,
                f"{bar.get_height():.0f}%",
                ha="center", va="bottom",
                fontsize=11, fontweight="bold")

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, "task_completion.png")
    plt.savefig(path, dpi=150)
    plt.close()
    print(f"[SAVED] {path}")


# ─────────────────────────────────────────
#  Chart 3: Detection accuracy per class
#  Shows how accurate YOLOv8 was for each
#  object category during field trials
# ─────────────────────────────────────────
def plot_detection_accuracy(df):
    # These columns must exist in test_results.csv
    classes = ["person", "bicycle", "car", "door",
               "stairs", "curb", "obstacle"]

    # Each column should be accuracy 0.0 – 1.0
    accuracy_cols = [f"acc_{c}" for c in classes]

    # Check columns exist
    available = [c for c in accuracy_cols if c in df.columns]
    if not available:
        print("[WARN] No accuracy columns found in CSV — skipping chart 3")
        return

    accuracies = [df[col].mean() * 100 for col in available]
    labels     = [c.replace("acc_", "") for c in available]

    # Colour bars by accuracy level
    colors = [COLOR_LOW if a >= 80
              else COLOR_MEDIUM if a >= 60
              else COLOR_HIGH
              for a in accuracies]

    fig, ax = plt.subplots(figsize=(9, 5))
    bars = ax.bar(labels, accuracies, color=colors, alpha=0.85, width=0.6)

    ax.set_title("YOLOv8 Detection Accuracy by Object Class",
                 fontsize=13, fontweight="bold")
    ax.set_ylabel("Accuracy (%)")
    ax.set_ylim(0, 110)

    for bar in bars:
        ax.text(bar.get_x() + bar.get_width()/2,
                bar.get_height() + 1.5,
                f"{bar.get_height():.1f}%",
                ha="center", va="bottom", fontsize=9)

    # Legend
    patches = [
        mpatches.Patch(color=COLOR_LOW,    label="≥ 80% (good)"),
        mpatches.Patch(color=COLOR_MEDIUM, label="60–80% (ok)"),
        mpatches.Patch(color=COLOR_HIGH,   label="< 60% (needs work)")
    ]
    ax.legend(handles=patches, loc="lower right")

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, "detection_accuracy.png")
    plt.savefig(path, dpi=150)
    plt.close()
    print(f"[SAVED] {path}")


# ─────────────────────────────────────────
#  Main — load CSV and run all charts
# ─────────────────────────────────────────
def main():
    if not os.path.exists(RESULTS_CSV):
        print(f"[ERROR] Cannot find {RESULTS_CSV}")
        print("        Run user testing first and save results to that path.")
        return

    print(f"[INFO] Loading {RESULTS_CSV}")
    df = pd.read_csv(RESULTS_CSV)
    print(f"[INFO] {len(df)} rows loaded")

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    plot_heart_rate(df)
    plot_task_completion(df)
    plot_detection_accuracy(df)

    print("[DONE] All charts saved to", OUTPUT_DIR)


if __name__ == "__main__":
    main()