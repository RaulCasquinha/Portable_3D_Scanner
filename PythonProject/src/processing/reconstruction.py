"""Surface reconstruction: point cloud → mesh STL.

Normal pipeline (grouped PCA):
  For each downsampled point, find neighbours within a search radius,
  keep only those whose ray direction is within dir_filter_deg of the
  query point's ray direction, then fit a plane to that filtered set
  via PCA.  The smallest eigenvector gives the surface normal; the
  sign is chosen so it faces the same side as the inherited ray hint.

Reconstruction: Screened Poisson with auto-scaled or user-set depth,
followed by optional Laplacian smoothing.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

import numpy as np

from src.data.point_cloud import PointCloud

_MAX_WIRE_EDGES = 3000


@dataclass
class ReconParams:
    depth: int        = 7      # Poisson depth (4-10); 0 = auto
    radius_pct: float = 10.0   # search radius as % of bbox diagonal
    max_nn: int       = 30     # max neighbours per point
    dir_filter_deg: float = 45.0  # only group points whose ray dirs agree within this angle
    smooth_iters: int = 5      # Laplacian smoothing passes (0 = off)


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _downsample_and_clean(points: np.ndarray, ray_normals: np.ndarray,
                           bbox_diag: float):
    """Voxel downsample + statistical outlier removal.

    Returns (clean_o3d_pcd, clean_points, clean_ray_normals, voxel_size).
    """
    import open3d as o3d

    pcd = o3d.geometry.PointCloud()
    pcd.points = o3d.utility.Vector3dVector(points.astype(np.float64))

    voxel_size = max(bbox_diag * 0.005, 1.0)
    pcd = pcd.voxel_down_sample(voxel_size)
    pcd, _ = pcd.remove_statistical_outlier(nb_neighbors=20, std_ratio=2.0)

    ds_pts = np.asarray(pcd.points, dtype=np.float64)
    if len(ds_pts) < 3:
        return pcd, ds_pts, np.empty((0, 3)), voxel_size

    # Inherit ray normals via nearest-neighbour from original cloud
    orig = o3d.geometry.PointCloud()
    orig.points = o3d.utility.Vector3dVector(points.astype(np.float64))
    tree = o3d.geometry.KDTreeFlann(orig)
    inherited = np.empty((len(ds_pts), 3), dtype=np.float64)
    for i, p in enumerate(pcd.points):
        _, idx, _ = tree.search_knn_vector_3d(p, 1)
        inherited[i] = ray_normals[idx[0]]

    return pcd, ds_pts, inherited, voxel_size


def _grouped_pca_normals(pcd, ds_pts: np.ndarray,
                          ray_normals: np.ndarray, params: ReconParams,
                          bbox_diag: float) -> np.ndarray:
    """Compute PCA normals with direction-filtered neighbourhood.

    For each point: find neighbours within radius, keep only those whose
    ray direction agrees within dir_filter_deg, fit a plane via PCA,
    orient result using the ray hint.
    """
    import open3d as o3d

    radius = max(bbox_diag * params.radius_pct / 100.0, 1.0)
    cos_thresh = math.cos(math.radians(params.dir_filter_deg)) if params.dir_filter_deg < 180.0 else -2.0
    tree = o3d.geometry.KDTreeFlann(pcd)
    normals_out = np.empty_like(ray_normals)

    for i in range(len(ds_pts)):
        k, idx, _ = tree.search_radius_vector_3d(pcd.points[i], radius)
        idx = list(idx)[:params.max_nn]

        # Direction filter: keep neighbours whose ray dir agrees with ours
        if cos_thresh > -1.0 and len(idx) >= 3:
            filtered = [j for j in idx
                        if float(np.dot(ray_normals[j], ray_normals[i])) >= cos_thresh]
            if len(filtered) >= 3:
                idx = filtered

        if len(idx) < 3:
            normals_out[i] = ray_normals[i]
            continue

        # PCA: smallest eigenvector of covariance = most orthogonal to surface
        neighbourhood = ds_pts[idx]
        centered = neighbourhood - neighbourhood.mean(axis=0)
        cov = (centered.T @ centered) / max(len(idx) - 1, 1)
        _, eigvecs = np.linalg.eigh(cov)  # columns sorted ascending by eigenvalue
        normal = eigvecs[:, 0]

        # Orient using ray hint
        if float(np.dot(normal, ray_normals[i])) < 0:
            normal = -normal
        normals_out[i] = normal

    return normals_out


def _prepare_pcd(cloud: PointCloud, params: ReconParams):
    """Full pre-processing: downsample → clean → grouped-PCA normals.

    Returns (o3d_pcd_with_normals, voxel_size, error_str).
    """
    try:
        import open3d as o3d
    except Exception as e:
        return None, 0.0, f"open3d import failed: {e}"

    points, ray_normals = cloud.to_numpy_with_normals()
    if len(points) < 20:
        return None, 0.0, f"Too few points ({len(points)}) — scan more first"

    bbox_diag = float(np.linalg.norm(points.max(axis=0) - points.min(axis=0)))
    pcd, ds_pts, inherited, voxel_size = _downsample_and_clean(
        points, ray_normals, bbox_diag)

    if len(ds_pts) < 20:
        return None, 0.0, f"Too few points after cleaning ({len(ds_pts)})"

    computed = _grouped_pca_normals(pcd, ds_pts, inherited, params, bbox_diag)
    pcd.normals = o3d.utility.Vector3dVector(computed)
    return pcd, voxel_size, ""


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def recommended_depth(cloud: PointCloud) -> int:
    """Suggest a Poisson depth based on cleaned point count."""
    points, _ = cloud.to_numpy_with_normals()
    if len(points) < 20:
        return 5
    n = len(points)
    return int(np.clip(math.floor(math.log2(max(n, 1) / 10)), 5, 7))


def compute_normals_preview(cloud: PointCloud,
                             params: ReconParams) -> tuple[np.ndarray, np.ndarray, str]:
    """Compute normals only (no Poisson).  Returns (points, normals, error)."""
    try:
        import open3d as o3d
    except Exception as e:
        return np.empty((0, 3)), np.empty((0, 3)), f"open3d import failed: {e}"

    pcd, _, err = _prepare_pcd(cloud, params)
    if pcd is None:
        return np.empty((0, 3)), np.empty((0, 3)), err

    pts  = np.asarray(pcd.points,  dtype=np.float32)
    nrms = np.asarray(pcd.normals, dtype=np.float32)
    return pts, nrms, ""


def reconstruct_mesh(cloud: PointCloud,
                     params: ReconParams | None = None):
    """Poisson reconstruction.  Returns (o3d mesh, error_str)."""
    import open3d as o3d

    if params is None:
        params = ReconParams()

    depth = params.depth if params.depth > 0 else recommended_depth(cloud)

    pcd, _, err = _prepare_pcd(cloud, params)
    if pcd is None:
        return None, err

    mesh, _ = o3d.geometry.TriangleMesh.create_from_point_cloud_poisson(
        pcd, depth=depth)

    if len(mesh.triangles) == 0:
        return None, "Mesh is empty — scan more coverage"

    if params.smooth_iters > 0:
        mesh = mesh.filter_smooth_laplacian(number_of_iterations=params.smooth_iters)
    mesh.compute_vertex_normals()
    return mesh, ""


def mesh_to_wireframe(mesh) -> tuple[np.ndarray, np.ndarray]:
    verts = np.asarray(mesh.vertices, dtype=np.float32)
    tris  = np.asarray(mesh.triangles)
    edges = np.vstack([tris[:, [0, 1]], tris[:, [1, 2]], tris[:, [0, 2]]])
    edges = np.sort(edges, axis=1)
    edges = np.unique(edges, axis=0)
    if len(edges) > _MAX_WIRE_EDGES:
        edges = edges[:: len(edges) // _MAX_WIRE_EDGES]
    return verts, edges


def save_mesh_stl(mesh, path: str) -> tuple[bool, str]:
    try:
        import open3d as o3d
    except Exception as e:
        return False, f"open3d import failed: {e}"
    if not o3d.io.write_triangle_mesh(path, mesh):
        return False, f"Open3D could not write to {path}"
    return True, f"Saved {len(np.asarray(mesh.triangles))} triangles → {path}"


def reconstruct_poisson(cloud: PointCloud, path: str,
                        params: ReconParams | None = None) -> tuple[bool, str]:
    mesh, err = reconstruct_mesh(cloud, params)
    if mesh is None:
        return False, err
    return save_mesh_stl(mesh, path)
