"""Generate netCDF-4 chunking explanation diagram.

Two isometric 3-D cubes side by side (portrait layout, B&W):
  Left  — index order: horizontal slabs with horizontal scan arrows on front face
  Right — chunked: subdivided into small 3-D sub-cubes
Rules: B&W only, max height 6.1 in, no caption inside figure.
"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# ── Isometric basis (standard: X→right-down, Y→up, Z→left-down) ─────────────
# Screen coords: right is +sx, up is +sy
# X grid → ( cos30,  -sin30) = (√3/2, -0.5)
# Y grid → (     0,       1)
# Z grid → (-cos30,  -sin30) = (-√3/2, -0.5)

_RX = np.array([ np.sqrt(3)/2, -0.5])
_RY = np.array([ 0.0,           1.0])
_RZ = np.array([-np.sqrt(3)/2, -0.5])


def iso(x, y, z, offset=np.zeros(2)):
    return x * _RX + y * _RY + z * _RZ + offset


def draw_index_cube(ax, N, offset, lw=0.7):
    """Draw the left 'index order' cube as a solid block.

    Shows 3 visible faces (front, right, top) as uniform grey panels,
    with horizontal slab-division lines on the front and right faces,
    and cell-division lines on the top face.
    Scan arrows are drawn on the front face for each slab row.
    """
    O = offset
    BASE_Z = 5

    # ── Draw the three solid faces of the whole cube ────────────────────────
    front_pts = np.array([
        iso(0, 0, 0, O), iso(N, 0, 0, O),
        iso(N, N, 0, O), iso(0, N, 0, O),
    ])
    right_pts = np.array([
        iso(N, 0, 0, O), iso(N, 0, N, O),
        iso(N, N, N, O), iso(N, N, 0, O),
    ])
    top_pts = np.array([
        iso(0, N, 0, O), iso(N, N, 0, O),
        iso(N, N, N, O), iso(0, N, N, O),
    ])
    ax.add_patch(mpatches.Polygon(right_pts, closed=True,
                 facecolor="0.65", edgecolor="black", lw=lw, zorder=BASE_Z))
    ax.add_patch(mpatches.Polygon(top_pts, closed=True,
                 facecolor="0.82", edgecolor="black", lw=lw, zorder=BASE_Z + 1))
    ax.add_patch(mpatches.Polygon(front_pts, closed=True,
                 facecolor="white", edgecolor="black", lw=lw, zorder=BASE_Z + 2))

    GZ = BASE_Z + 10  # grid lines always above all face panels

    # ── Horizontal slab dividers on front face ──────────────────────────────
    for iy in range(1, N):
        p0 = iso(0, iy, 0, O); p1 = iso(N, iy, 0, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=lw * 0.8, zorder=GZ)

    # ── Vertical cell lines on front face ───────────────────────────────────
    for ix in range(1, N):
        p0 = iso(ix, 0, 0, O); p1 = iso(ix, N, 0, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=lw * 0.4, zorder=GZ)

    # ── Horizontal slab dividers on right face ──────────────────────────────
    for iy in range(1, N):
        p0 = iso(N, iy, 0, O); p1 = iso(N, iy, N, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=lw * 0.8, zorder=GZ)

    # ── Depth lines on right face ───────────────────────────────────────────
    for iz in range(1, N):
        p0 = iso(N, 0, iz, O); p1 = iso(N, N, iz, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=lw * 0.4, zorder=GZ)

    # ── Top face grid lines ─────────────────────────────────────────────────
    for ix in range(1, N):
        p0 = iso(ix, N, 0, O); p1 = iso(ix, N, N, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=lw * 0.4, zorder=GZ)
    for iz in range(1, N):
        p0 = iso(0, N, iz, O); p1 = iso(N, N, iz, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=lw * 0.4, zorder=GZ)

    # ── Scan arrows: one per slab row, going left→right on front face ───────
    for iy in range(N):
        n_arr = 2
        for ia in range(n_arr):
            frac = (ia + 1.0) / (n_arr + 1)
            y_val = iy + frac
            p_l = iso(0.12, y_val, 0, O)
            p_r = iso(N - 0.12, y_val, 0, O)
            ax.annotate("",
                        xy=(p_r[0], p_r[1]), xytext=(p_l[0], p_l[1]),
                        arrowprops=dict(arrowstyle="-|>", color="black",
                                       lw=0.9, mutation_scale=7),
                        zorder=BASE_Z+2)


def draw_chunk_cube(ax, N, offset, lw=0.55):
    """Draw chunked cube as 3 solid face panels + grid lines overlay.

    No overlapping polygons — avoids all z-order artifacts.
    Grid lines give the visual impression of N×N×N sub-cubes.
    """
    O = offset
    GLW = lw * 0.65   # grid line width

    # ── Three solid face panels ───────────────────────────────────────────
    # front face (z=0)
    front = np.array([
        iso(0, 0, 0, O), iso(N, 0, 0, O),
        iso(N, N, 0, O), iso(0, N, 0, O),
    ])
    # right face (x=N)
    right = np.array([
        iso(N, 0, 0, O), iso(N, 0, N, O),
        iso(N, N, N, O), iso(N, N, 0, O),
    ])
    # top face (y=N)
    top = np.array([
        iso(0, N, 0, O), iso(N, N, 0, O),
        iso(N, N, N, O), iso(0, N, N, O),
    ])
    ax.add_patch(mpatches.Polygon(right, closed=True,
                 facecolor="0.68", edgecolor="black", lw=lw, zorder=20))
    ax.add_patch(mpatches.Polygon(top, closed=True,
                 facecolor="0.84", edgecolor="black", lw=lw, zorder=21))
    ax.add_patch(mpatches.Polygon(front, closed=True,
                 facecolor="white", edgecolor="black", lw=lw, zorder=22))

    GZ = 30  # grid lines above all chunk cube faces

    # ── Grid lines on front face ─────────────────────────────────────────
    for ix in range(1, N):
        p0 = iso(ix, 0, 0, O); p1 = iso(ix, N, 0, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=GLW, zorder=GZ)
    for iy in range(1, N):
        p0 = iso(0, iy, 0, O); p1 = iso(N, iy, 0, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=GLW, zorder=GZ)

    # ── Grid lines on right face ─────────────────────────────────────────
    for iy in range(1, N):
        p0 = iso(N, iy, 0, O); p1 = iso(N, iy, N, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=GLW, zorder=GZ)
    for iz in range(1, N):
        p0 = iso(N, 0, iz, O); p1 = iso(N, N, iz, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=GLW, zorder=GZ)

    # ── Grid lines on top face ───────────────────────────────────────────
    for ix in range(1, N):
        p0 = iso(ix, N, 0, O); p1 = iso(ix, N, N, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=GLW, zorder=GZ)
    for iz in range(1, N):
        p0 = iso(0, N, iz, O); p1 = iso(N, N, iz, O)
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], "k-", lw=GLW, zorder=GZ)


# ── Figure ────────────────────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(7, 5.0))
ax.set_aspect("equal")
ax.axis("off")

N = 5

# Left cube origin
OL = np.array([0.0, 0.0])
draw_index_cube(ax, N, OL)

# Right cube: place to the right of the left cube with a gap
# Rightmost 2-D point of left cube is iso(N, 0, 0, OL)
right_of_left = iso(N, 0, 0, OL)
gap = 0.9
OR = np.array([right_of_left[0] + gap, OL[1]])

draw_chunk_cube(ax, N, OR)

# Labels
lbl_l = iso(N/2, -0.7, N/2, OL)
ax.text(lbl_l[0], lbl_l[1], "index order", ha="center", va="top", fontsize=11)

lbl_r = iso(N/2, -0.7, N/2, OR)
ax.text(lbl_r[0], lbl_r[1], "chunked", ha="center", va="top", fontsize=11)

# Auto-fit axes
pts = [iso(x, y, z, O)
       for O in [OL, OR]
       for x in [0, N] for y in [0, N] for z in [0, N]]
pts = np.array(pts)
pad = 0.4
ax.set_xlim(pts[:,0].min() - pad, pts[:,0].max() + pad)
ax.set_ylim(pts[:,1].min() - 1.0, pts[:,1].max() + pad)

plt.tight_layout(pad=0.2)
plt.savefig("netcdf4_chunking_diagram.png", dpi=150, bbox_inches="tight")
print("Saved netcdf4_chunking_diagram.png")
