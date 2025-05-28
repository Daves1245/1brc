#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // For memset
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAP_CAPACITY (1 << 14)
// TODO determine tighter fit
#define MAX_CITIES 100000
#define MAX_CITY_NAME_LEN 16

struct map_item {
  int index;
  int hash;
};
struct map_item map[MAP_CAPACITY];

struct city {
  char name[MAX_CITY_NAME_LEN];
  float max;
  float min;
  float sum;
  int cnt;
};

int city_index = 0;
struct city cities[MAX_CITIES];

inline int m31s(const char *key, const char *end) {
  int h = 0;
  while (key < end) {
    h *= 32;
    h += (unsigned char)*key - 'A';
    key++;
  }
  return h;
}

int get_map_index(int hash) {
  int h = hash;
  while (map[h].index && (map[h].hash != hash)) {
    h = (h + 1) & (MAP_CAPACITY - 1);
  }

  if (!map[h].index) {
    map[h].index = city_index++;
    map[h].hash = hash;
    // printf("\nNEW CITY ENTRY: hash=%d, map_index=%d, city_index=%d", hash, h,
    // map[h].index);
  } else {
    // printf("\nEXISTING CITY: hash=%d, map_index=%d, city_index=%d", hash, h,
    // map[h].index);
  }
  return h;
}

int cmp_cities(const void *a, const void *b) {
  return strcmp(((const struct city *)a)->name, ((const struct city *)b)->name);
}

int main() {
  int fd;
  char *start, *peek;
  struct stat st;

  fd = open("/tmp/measurements10k.txt", O_RDONLY);
  fstat(fd, &st);
  start = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  peek = start;
  // printf("head measurements.txt: %.50s...\n", peek);

  char *test = peek;
  char *city_start, *city_end;

  struct {
    int is_city;
    unsigned hash;
    int measurement;
    float measurement_frac;
    int neg;
    int is_frac;
    float frac_mag;
    int padding;
  } state = {0};

  state.is_city = 1;
  city_start = peek;

  int map_index;

  for (; peek - start < st.st_size; peek++) {
    if (*peek == ';') {
      state.is_city = 0;
      city_end = peek;
      // printf("\nchar: ';' - end of city name, hash=%d", state.hash);
      peek++;
      if (*peek == '-') {
	state.neg = 1;
	// printf("\nchar: '-' - negative");
      }
      continue;
    }

    if (*peek == '\n') {
      float final_measurement = state.measurement + state.measurement_frac;
      if (state.neg) final_measurement = -final_measurement;

      map_index = get_map_index(state.hash & (MAP_CAPACITY - 1));
      int city_idx = map[map_index].index;

      int city_name_len = city_end - city_start;
      if (city_name_len >= MAX_CITY_NAME_LEN) {
	city_name_len = MAX_CITY_NAME_LEN - 1;
      }

      if (cities[city_idx].cnt == 0) {
	memcpy(cities[city_idx].name, city_start, city_name_len);
	cities[city_idx].name[city_name_len] = '\0';
	cities[city_idx].min = final_measurement;
	cities[city_idx].max = final_measurement;
	cities[city_idx].sum = final_measurement;
	cities[city_idx].cnt = 1;
      } else {
	cities[city_idx].min = cities[city_idx].min < final_measurement
				   ? cities[city_idx].min
				   : final_measurement;
	cities[city_idx].max = cities[city_idx].max > final_measurement
				   ? cities[city_idx].max
				   : final_measurement;
	cities[city_idx].sum += final_measurement;
	cities[city_idx].cnt++;
      }

      /*
      printf("\nline %d: [%.*s]", line_count++, (int)(line_end - line_start),
	     line_start);
      printf("\nparsed: int_part=%d, frac_part=%.6f, is_negative=%d",
	     state.measurement, state.measurement_frac, state.neg);
      printf("\nhash: %d, map_index: %d, city_index: %d", state.hash, map_index,
	     city_idx);
      printf("\ncity: '%s', count: %d, min: %.1f, max: %.1f, avg: %.1f",
	     cities[city_idx].name, cities[city_idx].cnt, cities[city_idx].min,
	     cities[city_idx].max, cities[city_idx].sum / cities[city_idx].cnt);
      printf("\nfinal value: %.1f", final_measurement);
      printf("\n--------------------------------------------------");
      */

      memset(&state, 0, sizeof(state));
      state.frac_mag = 1.0;
      state.is_city = 1;
      city_start = peek + 1;
      continue;
    }

    if (*peek == '.') {
      state.is_frac = 1;
      // printf("\nchar: '.' - switching to fractional part");
      continue;
    }

    if (state.is_city == 1) {
      state.hash *= 31;
      state.hash += (unsigned char)*peek - 'A';
      // printf("\nchar: '%c', city_hash=%d", *peek, state.hash);
    } else {
      if (state.is_frac == 0) {
	state.measurement *= 10;
	state.measurement += (unsigned char)*peek - '0';
	//	printf("\nchar: '%c', int_part=%d", *peek, state.measurement);
      } else {
	state.frac_mag *= 10.0;
	float digit_value = ((unsigned char)*peek - '0') / state.frac_mag;
	state.measurement_frac += digit_value;
	// printf("\nchar: '%c', digit_value=%.6f, frac_part=%.6f", *peek,
	// digit_value, state.measurement_frac);
      }
    }
  }

  qsort(cities, MAX_CITIES, sizeof(struct city), cmp_cities);

  printf("{ ");
  for (int i = 0; i < MAX_CITIES; i++) {
    struct city *it = &cities[i];
    if (!it->cnt) {
      continue;
    }
    printf("%s=", it->name);
    printf("%.1f/", it->min);
    printf("%.1f/", it->sum / it->cnt);
    printf("%.1f", it->max);
    if (i != city_index) {
      printf(", ");
    }
  }
  puts("}");

  munmap(start, st.st_size);
  close(fd);
  return 0;
}
