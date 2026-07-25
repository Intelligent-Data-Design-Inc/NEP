"""Test common visualization output constraints and metadata generation.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
from pathlib import Path
from tempfile import TemporaryDirectory

import matplotlib.image as mpimg
import matplotlib.pyplot as plt
import numpy as np

from plot_common import save_with_metadata
from verify_viz_artifacts import validate_artifacts


def main():
    fig, ax = plt.subplots(figsize=(4.0, 3.0))
    ax.plot([0, 1, 2], [0, 1, 0], color="black")
    ax.set_xlabel("Sample")
    ax.set_ylabel("Value")

    png_path, metadata_path = save_with_metadata(
        fig,
        "viz_plot_common_test",
        "Visualization helper test",
        "Synthetic black line data verify PNG and companion metadata generation.",
        "A black line rises from zero to one and returns to zero.",
    )
    plt.close(fig)

    if not png_path.is_file() or not metadata_path.is_file():
        raise RuntimeError("expected visualization artifacts were not created")

    image = mpimg.imread(png_path)
    max_width = int(8.0 * 150)
    max_height = int(6.1 * 150)
    if image.shape[1] > max_width or image.shape[0] > max_height:
        raise RuntimeError(f"PNG dimensions exceed limits: {image.shape[1]}x{image.shape[0]}")
    if image.ndim == 3 and image.shape[2] >= 3:
        if not (
            (abs(image[:, :, 0] - image[:, :, 1]) < 1e-6).all()
            and (abs(image[:, :, 1] - image[:, :, 2]) < 1e-6).all()
        ):
            raise RuntimeError("PNG contains non-grayscale pixels")

    expected = [
        "title: Visualization helper test",
        "caption: Synthetic black line data verify PNG and companion metadata generation.",
        "alt_text: A black line rises from zero to one and returns to zero.",
    ]
    actual = Path(metadata_path).read_text(encoding="utf-8").splitlines()
    if actual != expected:
        raise RuntimeError(f"unexpected metadata content: {actual!r}")

    invalid_cases = [
        ((4.0, 3.0), "", "caption", "alt text"),
        ((4.0, 3.0), "title", "word " * 76, "alt text"),
        ((8.1, 3.0), "title", "caption", "alt text"),
        ((4.0, 6.2), "title", "caption", "alt text"),
    ]
    for index, (figsize, title, caption, alt_text) in enumerate(invalid_cases):
        invalid_fig = plt.figure(figsize=figsize)
        try:
            save_with_metadata(
                invalid_fig,
                f"viz_plot_common_invalid_{index}",
                title,
                caption,
                alt_text,
            )
        except ValueError:
            pass
        else:
            raise RuntimeError(f"invalid helper input was accepted: case {index}")
        finally:
            plt.close(invalid_fig)

    with TemporaryDirectory() as temporary_directory:
        artifact_dir = Path(temporary_directory)
        negative_cases = {
            "missing_metadata": lambda: (artifact_dir / "missing_metadata_metadata.txt").unlink(),
            "malformed_metadata": lambda: (artifact_dir / "malformed_metadata_metadata.txt").write_text(
                "caption: invalid order\ntitle: invalid order\nalt_text: invalid order\n",
                encoding="utf-8",
            ),
            "long_caption": lambda: (artifact_dir / "long_caption_metadata.txt").write_text(
                f"title: title\ncaption: {'word ' * 76}\nalt_text: alt text\n",
                encoding="utf-8",
            ),
            "oversized": lambda: plt.imsave(
                artifact_dir / "oversized.png", np.zeros((916, 1201)), cmap="gray"
            ),
            "color": lambda: plt.imsave(
                artifact_dir / "color.png", np.array([[[1.0, 0.0, 0.0]]])
            ),
        }
        for basename, corrupt in negative_cases.items():
            fig = plt.figure(figsize=(1.0, 1.0))
            try:
                save_with_metadata(fig, artifact_dir / basename, "title", "caption", "alt text")
            finally:
                plt.close(fig)
            corrupt()
            try:
                validate_artifacts(artifact_dir, [basename])
            except RuntimeError:
                pass
            else:
                raise RuntimeError(f"artifact verifier accepted invalid case: {basename}")


if __name__ == "__main__":
    main()
