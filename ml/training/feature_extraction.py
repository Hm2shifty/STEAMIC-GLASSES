# ─────────────────────────────────────────
#  EchoNav — ml/training/feature_extraction.py
#  Prepares raw images from COCO / Open Images
#  for fine-tuning YOLOv8 on navigation
#  relevant objects (people, vehicles,
#  doors, stairs, curbs, obstacles).
#
#  Steps:
#  1. Load images from input folder
#  2. Resize to 640x640 (YOLO input size)
#  3. Apply augmentations (flip, brightness,
#     rotation) to increase dataset variety
#  4. Save processed images to output folder
#     ready for Ultralytics YOLO training
#
#  Run this ONCE before training:
#  python feature_extraction.py
# ─────────────────────────────────────────

import os
import cv2
import numpy as np
import random

# ── Paths ─────────────────────────────────
# Put your raw downloaded images here
INPUT_DIR  = "data/raw"
# Processed images ready for YOLO go here
OUTPUT_DIR = "data/processed"

# ── Image size ────────────────────────────
# YOLOv8 expects 640x640
TARGET_SIZE = (640, 640)

# ── Augmentation settings ─────────────────
# How many augmented copies to make per image
AUGMENT_COPIES = 3

# Brightness adjustment range (0.5 = darker, 1.5 = brighter)
BRIGHTNESS_RANGE = (0.5, 1.5)

# Rotation range in degrees
ROTATION_RANGE = (-15, 15)


# ─────────────────────────────────────────
#  Augmentation functions
#  Each takes an image (numpy array) and
#  returns a modified copy
# ─────────────────────────────────────────

def resize_image(img):
    """Resize to 640x640 — required for YOLO"""
    return cv2.resize(img, TARGET_SIZE)


def flip_horizontal(img):
    """Mirror the image left-right"""
    return cv2.flip(img, 1)


def adjust_brightness(img):
    """Randomly make image brighter or darker"""
    factor = random.uniform(*BRIGHTNESS_RANGE)
    # Convert to float, scale, clip back to 0-255
    bright = np.clip(img.astype(np.float32) * factor, 0, 255)
    return bright.astype(np.uint8)


def rotate_image(img):
    """Randomly rotate the image slightly"""
    angle  = random.uniform(*ROTATION_RANGE)
    h, w   = img.shape[:2]
    center = (w // 2, h // 2)

    # Get rotation matrix and apply it
    matrix = cv2.getRotationMatrix2D(center, angle, 1.0)
    rotated = cv2.warpAffine(img, matrix, (w, h),
                              borderMode=cv2.BORDER_REFLECT)
    return rotated


def add_noise(img):
    """
    Add a tiny amount of random noise.
    Helps the model handle low-quality
    camera frames from the OV2640.
    """
    noise  = np.random.randint(0, 15, img.shape, dtype=np.uint8)
    noisy  = cv2.add(img, noise)
    return noisy


# ─────────────────────────────────────────
#  Main processing pipeline
# ─────────────────────────────────────────

def process_image(img):
    """
    Apply the full augmentation pipeline
    to one image. Returns a list of
    augmented versions including the original.
    """
    # Always include the clean resized version
    base = resize_image(img)
    versions = [base]

    for _ in range(AUGMENT_COPIES):
        aug = base.copy()

        # Randomly apply each augmentation
        if random.random() > 0.5:
            aug = flip_horizontal(aug)
        if random.random() > 0.5:
            aug = adjust_brightness(aug)
        if random.random() > 0.5:
            aug = rotate_image(aug)
        if random.random() > 0.3:
            aug = add_noise(aug)

        versions.append(aug)

    return versions


def run_extraction():
    """
    Walks INPUT_DIR, processes every image,
    saves results to OUTPUT_DIR.
    """
    # Make sure output folder exists
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Supported image extensions
    extensions = (".jpg", ".jpeg", ".png", ".bmp")

    # Count files for progress reporting
    all_files = [
        f for f in os.listdir(INPUT_DIR)
        if f.lower().endswith(extensions)
    ]
    total = len(all_files)

    if total == 0:
        print(f"[ERROR] No images found in {INPUT_DIR}")
        return

    print(f"[INFO] Found {total} images in {INPUT_DIR}")
    print(f"[INFO] Generating {AUGMENT_COPIES + 1} versions per image...")

    saved = 0

    for i, filename in enumerate(all_files):
        input_path = os.path.join(INPUT_DIR, filename)

        # Load image
        img = cv2.imread(input_path)
        if img is None:
            print(f"[WARN] Could not read {filename} — skipping")
            continue

        # Process and save all augmented versions
        versions = process_image(img)
        base_name = os.path.splitext(filename)[0]

        for j, version in enumerate(versions):
            out_name = f"{base_name}_v{j}.jpg"
            out_path = os.path.join(OUTPUT_DIR, out_name)
            cv2.imwrite(out_path, version)
            saved += 1

        # Print progress every 100 images
        if (i + 1) % 100 == 0:
            print(f"[INFO] Processed {i + 1}/{total} images...")

    print(f"[DONE] Saved {saved} processed images to {OUTPUT_DIR}")


# ─────────────────────────────────────────
#  Run when executed directly
# ─────────────────────────────────────────
if __name__ == "__main__":
    run_extraction()