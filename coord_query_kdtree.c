#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "record.h"
#include "coord_query.h"


struct kd_node {
  const struct record *record;
  struct kd_node *left;
  struct kd_node *right;
  int axis;
};

struct kd_data {
  struct kd_node *root;
};

static int cmp_lon(const void *a, const void *b) {
  const struct record *r1 = *(const struct record **)a;
  const struct record *r2 = *(const struct record **)b;
  if (r1->lon < r2->lon) return -1;
  if (r1->lon > r2->lon) return 1;
  return 0;
}

static int cmp_lat(const void *a, const void *b) {
  const struct record *r1 = *(const struct record **)a;
  const struct record *r2 = *(const struct record **)b;
  if (r1->lat < r2->lat) return -1;
  if (r1->lat > r2->lat) return 1;
  return 0;
}

static struct kd_node* build_kd_tree(const struct record **refs, int n, int depth) {
  if (n <= 0) return NULL;

  int axis = depth % 2;

  if (axis == 0) {
    qsort(refs, n, sizeof(const struct record *), cmp_lon);
  } else {
    qsort(refs, n, sizeof(const struct record *), cmp_lat);
  }

  int mid = n / 2;

  struct kd_node *node = malloc(sizeof(struct kd_node));
  assert(node != NULL);

  node->record = refs[mid];
  node->axis = axis;

  node->left = build_kd_tree(refs, mid, depth + 1);
  node->right = build_kd_tree(refs + mid + 1, n - mid - 1, depth + 1);

  return node;
}

struct kd_data* mk_kdtree(struct record* rs, int n) {
  const struct record **refs = malloc(n * sizeof(const struct record *));
  assert(refs != NULL);

  for (int i = 0; i < n; i++) {
    refs[i] = &rs[i];
  }

  struct kd_data *data = malloc(sizeof(struct kd_data));
  assert(data != NULL);

  data->root = build_kd_tree(refs, n, 0);

  free(refs);
  return data;
}

static void free_kd_node(struct kd_node *node) {
  if (node == NULL) return;
  free_kd_node(node->left);
  free_kd_node(node->right);
  free(node);
}

void free_kdtree(struct kd_data *data) {
  if (data == NULL) return;
  free_kd_node(data->root);
  free(data);
}
static double dist_sq(double lon1, double lat1, double lon2, double lat2) {
  double dx = lon1 - lon2;
  double dy = lat1 - lat2;
  return dx * dx + dy * dy;
}

static void kd_search(const struct kd_node *node, double lon, double lat,
                      const struct record **best_record, double *best_dist_sq) {
  if (node == NULL) return;

  double d = dist_sq(node->record->lon, node->record->lat, lon, lat);
  if (*best_record == NULL || d < *best_dist_sq) {
    *best_dist_sq = d;
    *best_record = node->record;
  }

  double diff = (node->axis == 0) ? (lon - node->record->lon)
                                  : (lat - node->record->lat);

  const struct kd_node *first = (diff < 0) ? node->left : node->right;
  const struct kd_node *second = (diff < 0) ? node->right : node->left;

  kd_search(first, lon, lat, best_record, best_dist_sq);

  if (diff * diff < *best_dist_sq) {
    kd_search(second, lon, lat, best_record, best_dist_sq);
  }
}

const struct record* lookup_kdtree(struct kd_data *data, double lon, double lat) {
  if (data == NULL || data->root == NULL) return NULL;

  const struct record *best_record = NULL;
  double best_dist_sq = INFINITY;

  kd_search(data->root, lon, lat, &best_record, &best_dist_sq);
  return best_record;
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}