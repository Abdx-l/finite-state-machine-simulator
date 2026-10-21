#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 4096
#define LINE_SIZE 8192

typedef struct {
    int next[2];
    unsigned char active;
} State;

typedef struct {
    State *states;
    int count;
    int start;
    int current;
} Machine;

static void help(void) {
    puts("Commands: set STATE SYMBOL TARGET | start STATE | reset |");
    puts("          step SYMBOL | run BITS | print | reachable | prune | help | quit");
}

static int parse_int(const char *s, int *out) {
    char *end;
    long value;
    if (!s || !*s) return 0;
    errno = 0;
    value = strtol(s, &end, 10);
    if (errno || *end || value < 0 || value > INT_MAX) return 0;
    *out = (int)value;
    return 1;
}

static int valid_state(const Machine *m, const char *s, int *out) {
    return parse_int(s, out) && *out < m->count && m->states[*out].active;
}

static int symbol(const char *s, int *out) {
    if (!s || s[1] || (s[0] != '0' && s[0] != '1')) return 0;
    *out = s[0] - '0';
    return 1;
}

static int no_extra(char *extra) { return extra == NULL; }

static void mark_reachable(const Machine *m, unsigned char *seen) {
    int *queue = malloc((size_t)m->count * sizeof(*queue));
    int head = 0, tail = 0;
    if (!queue) {
        fputs("out of memory\n", stderr);
        exit(2);
    }
    seen[m->start] = 1;
    queue[tail++] = m->start;
    while (head < tail) {
        int from = queue[head++];
        for (int bit = 0; bit < 2; bit++) {
            int to = m->states[from].next[bit];
            if (to >= 0 && to < m->count && m->states[to].active && !seen[to]) {
                seen[to] = 1;
                queue[tail++] = to;
            }
        }
    }
    free(queue);
}

static unsigned char *reachability(const Machine *m) {
    unsigned char *seen = calloc((size_t)m->count, 1);
    if (!seen) {
        fputs("out of memory\n", stderr);
        exit(2);
    }
    mark_reachable(m, seen);
    return seen;
}

static void print_machine(const Machine *m) {
    printf("start=%d current=%d\n", m->start, m->current);
    for (int i = 0; i < m->count; i++) {
        if (m->states[i].active)
            printf("%d: 0->%d 1->%d\n", i, m->states[i].next[0],
                   m->states[i].next[1]);
    }
}

static void unreachable(const Machine *m, int prune) {
    unsigned char *seen = reachability(m);
    int total = 0;
    for (int i = 0; i < m->count; i++) {
        if (m->states[i].active && !seen[i]) {
            if (!prune) printf("%s%d", total ? " " : "unreachable: ", i);
            else m->states[i].active = 0;
            total++;
        }
    }
    if (prune) printf("pruned %d state(s)\n", total);
    else if (total) putchar('\n');
    else puts("unreachable: none");
    free(seen);
}

static void execute(Machine *m, char *line, int *done) {
    char *cmd = strtok(line, " \t\r\n");
    char *a, *b, *c, *extra;
    int from, to, bit;
    if (!cmd || cmd[0] == '#') return;
    a = strtok(NULL, " \t\r\n");
    b = strtok(NULL, " \t\r\n");
    c = strtok(NULL, " \t\r\n");
    extra = strtok(NULL, " \t\r\n");

    if (!strcmp(cmd, "quit") && !a) { *done = 1; return; }
    if (!strcmp(cmd, "help") && !a) { help(); return; }
    if (!strcmp(cmd, "print") && !a) { print_machine(m); return; }
    if (!strcmp(cmd, "reachable") && !a) { unreachable(m, 0); return; }
    if (!strcmp(cmd, "prune") && !a) { unreachable(m, 1); return; }
    if (!strcmp(cmd, "reset") && !a) {
        m->current = m->start;
        printf("current=%d\n", m->current);
        return;
    }
    if (!strcmp(cmd, "start") && valid_state(m, a, &from) && !b) {
        m->start = m->current = from;
        printf("start=%d current=%d\n", from, from);
        return;
    }
    if (!strcmp(cmd, "set") && valid_state(m, a, &from) &&
        symbol(b, &bit) && valid_state(m, c, &to) && no_extra(extra)) {
        m->states[from].next[bit] = to;
        printf("%d: %d->%d\n", from, bit, to);
        return;
    }
    if (!strcmp(cmd, "step") && symbol(a, &bit) && !b) {
        m->current = m->states[m->current].next[bit];
        printf("current=%d\n", m->current);
        return;
    }
    if (!strcmp(cmd, "run") && a && !b) {
        for (const char *p = a; *p; p++) {
            if (*p != '0' && *p != '1') {
                puts("error: run expects only 0 and 1");
                return;
            }
        }
        for (const char *p = a; *p; p++)
            m->current = m->states[m->current].next[*p - '0'];
        printf("current=%d\n", m->current);
        return;
    }
    puts("error: invalid command or inactive state; type help");
}

int main(int argc, char **argv) {
    int count = 6, done = 0;
    char line[LINE_SIZE];
    Machine m;
    if (argc > 2 || (argc == 2 && (!parse_int(argv[1], &count) ||
                                    count < 1 || count > MAX_STATES))) {
        fprintf(stderr, "usage: %s [states: 1..%d]\n", argv[0], MAX_STATES);
        return 1;
    }
    m.states = calloc((size_t)count, sizeof(*m.states));
    if (!m.states) { fputs("out of memory\n", stderr); return 2; }
    m.count = count;
    m.start = m.current = 0;
    for (int i = 0; i < count; i++) {
        m.states[i].active = 1;
        m.states[i].next[0] = m.states[i].next[1] = i;
    }
    while (!done && fgets(line, sizeof(line), stdin)) {
        if (!strchr(line, '\n') && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            puts("error: command too long");
            continue;
        }
        execute(&m, line, &done);
    }
    free(m.states);
    return 0;
}

