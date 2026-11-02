# processing — Mesh reconstruction

Converts the accumulated point cloud into a triangle mesh using Screened Poisson Surface Reconstruction.

## Files

| File | Description |
|------|-------------|
| `reconstruction.py` | All reconstruction logic: `ReconParams`, normal estimation, Poisson solve, STL export |

## API

| Function / Class | Description |
|-----------------|-------------|
| `ReconParams` | Dataclass holding all tunable parameters |
| `recommended_depth(cloud)` | Suggests Poisson depth from point count (range 5–7) |
| `compute_normals_preview(cloud, params)` | Computes grouped-PCA normals only; returns `(pts, normals, error)` for viewport preview |
| `reconstruct_mesh(cloud, params)` | Full pipeline → Open3D `TriangleMesh` |
| `mesh_to_wireframe(mesh)` | Extracts unique edges (capped at 3000) for DearPyGUI display |
| `save_mesh_stl(mesh, path)` | Writes STL via Open3D |

## ReconParams fields

| Field | Default | Meaning |
|-------|---------|---------|
| `depth` | 0 | Poisson octree depth; 0 = auto from `recommended_depth()` |
| `radius_pct` | 10.0 | Normal search radius as % of bounding-box diagonal |
| `max_nn` | 30 | Max neighbours per point for PCA |
| `dir_filter_deg` | 45.0 | Only group neighbours whose ray direction agrees within this angle |
| `smooth_iters` | 5 | Laplacian smoothing passes (0 = off) |

## Pipeline

```
PointCloud
  │
  ├─► voxel downsample (0.5 % bbox diagonal)
  ├─► statistical outlier removal (20 neighbours, 2σ)
  ├─► NN-inherit ray normals from original cloud
  ├─► grouped-PCA normal refinement (direction-filtered neighbourhood)
  ├─► Screened Poisson (auto or user depth)
  ├─► Laplacian smoothing
  └─► TriangleMesh (→ wireframe for display, → STL for export)
```

## Grouped-PCA normal estimation

For each downsampled point:
1. Find all neighbours within `radius_pct`% of the bounding-box diagonal
2. Keep only those whose ray direction agrees within `dir_filter_deg`
3. Fit a plane to the filtered set via PCA (smallest eigenvector = surface normal)
4. Orient the result so it agrees with the inherited ray hint
