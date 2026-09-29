#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "record.h"
#include "id_query.h"

struct index_record {
  int64_t osm_id;
  const struct record *record;
};

struct indexed_data {
  struct index_record *irs;
  int n;
};


int compare(const void *a, const void *b) {
  const struct index_record *x = a;
  const struct index_record *y = b;
  if (x->osm_id < y->osm_id) {
    return -1;
  }
  if (x->osm_id > y->osm_id) {
    return 1;
  }
  return 0;
}

struct indexed_data* mk_binsort(struct record* rs, int n) {
  struct indexed_data *data = malloc(sizeof(struct indexed_data));
  data->n = n;
  data->irs = malloc(n * sizeof(struct index_record));

  for (int i = 0; i < n; i++) {
    data->irs[i].osm_id = rs[i].osm_id;
    data->irs[i].record = &rs[i];
  }

  qsort(data->irs, n, sizeof(struct index_record), compare);
  return data;
}

void free_binsort(struct indexed_data* data) {
  free(data->irs);
  free(data);
}

const struct record* lookup_binsort(struct indexed_data *data, int64_t needle) {
  int low = 0;
  int high = data->n - 1;

  while (low <= high) {
    int mid = (low + high) / 2;
    if (data->irs[mid].osm_id == needle) {
      return data->irs[mid].record;
    }
    if (data->irs[mid].osm_id < needle) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }
  return NULL;
}

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                    (mk_index_fn)mk_binsort,
                    (free_index_fn)free_binsort,
                    (lookup_fn)lookup_binsort);
}