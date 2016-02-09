#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define STKSIZE 256

void error(const char *s);
typedef struct state {
    int ch;
    struct state *out;
    struct state *out1;
}state;

typedef struct nfa_frag {
    state *start_state;
    state *last_state;
}nfa_frag;

typedef struct list {
    int num;
    state *state_arr[STKSIZE];
}list;

list g_list1, g_list2;
enum {SPLIT=128, EMPTY, ACCEPT};
state accept_state = { ACCEPT, NULL, NULL };

char *reg2post(char *reg)
{
    int ndot, nalt;
    char ch;
    char *post, *buf;
    typedef struct bracket {
        int ndot;
        int nalt;
    }bracket;

    ndot = nalt = 0;
    bracket stack[STKSIZE], *sp;
    memset(stack, 0, sizeof(stack));
    sp = stack;
    post = calloc(STKSIZE, sizeof(char));
    buf = post;
    while (ch = *reg++) {
        switch (ch) {
            case '(':
                if (ndot == 2) {
                    ndot--;
                    *post++ = '.';
                }
                if (sp >= stack+STKSIZE)
                    return NULL;
                sp->ndot = ndot;
                sp->nalt = nalt;
                sp++;
                ndot = 0;
                nalt = 0;
                break;
            case '|':
                if (ndot == 0) 
                    return NULL;
                while (--ndot > 0) 
                    *post++ = '.';
                nalt++;
                break;
            case ')':
                if (ndot == 0) 
                    return NULL;
                if (sp == stack)
                    return NULL;
                while (--ndot > 0)
                    *post++ = '.';
                for (; nalt > 0; nalt--)
                    *post++ = '|';
                --sp;
                nalt = sp->nalt;
                ndot = sp->ndot;
                ndot++;
                break;
            case '+':
            case '?':
            case '*':
                if (ndot == 0) 
                    return NULL;
                *post++ = ch;
                break;
            default:
                if (ndot == 2) {
                    ndot--;
                    *post++ = '.';
                }
                ndot++;
                *post++ = ch;
                break;
        }
    }
    if (sp != stack)
        return NULL;
    while (--ndot > 0) 
        *post++ = '.';
    for (; nalt > 0; nalt--)
        *post++ = '|';
    *post = 0;
    return buf;
}

state *post2nfa(char *postfix)
{
    nfa_frag stack[STKSIZE], *newf, *sp, f1, f2;
    state *news, *news1, *news2;
    char ch;

#define push(f) (*sp++=f)
#define pop()   (*--sp)
    sp = stack;
    while (ch = *postfix++) {
        switch (ch) {
            case '|':
                news = calloc(1, sizeof(state));
                newf = calloc(1, sizeof(nfa_frag));
                f2 = pop();
                f1 = pop();
                news->ch = SPLIT;
                news->out = f1.start_state;
                news->out1 = f2.start_state;
                newf->start_state = news;
                news = calloc(1, sizeof(state));
                news->ch = EMPTY;
                f1.last_state->out = news;
                f2.last_state->out = news;
                newf->last_state = news;
                push(*newf);
                break;
            case '.':
                f2 = pop();
                f1 = pop();
                newf = calloc(1, sizeof(nfa_frag));
                newf->start_state = f1.start_state;
                newf->last_state = f2.last_state;
                f1.last_state->out = f2.start_state;
                push(*newf);
                break;
            case '+':
                f1 = pop();
                news1 = calloc(1, sizeof(state));
                news1->ch = EMPTY;
                news = calloc(1, sizeof(state));
                newf = calloc(1, sizeof(nfa_frag));
                news->ch = SPLIT;
                news->out = f1.start_state;
                news->out1 = news1;
                f1.last_state->out = news;
                newf->start_state = f1.start_state;
                newf->last_state = news1;
                push(*newf);
                break;
            case '?':
                f1 = pop();
                news = calloc(1, sizeof(state));
                news->ch = SPLIT;
                news->out = f1.start_state;
                newf = calloc(1, sizeof(nfa_frag));
                newf->start_state = news;
                
                news1 = calloc(1, sizeof(state));
				news1->ch = EMPTY;
                news->out1 = news1;
                f1.last_state->out = news1;
                newf->last_state = news1;
                push(*newf);
                break;
            case '*':
                f1 = pop();
				news = calloc(1, sizeof(state));	
				news1 = calloc(1, sizeof(state));	
				news2 = calloc(1, sizeof(state));	
				news->out = f1.start_state;
				news->out1 = news1;
				news->ch = SPLIT;
				f1.last_state->out = news1;
				news1->out = f1.start_state;
				news1->out1 = news2;
				news1->ch = SPLIT;
				news2->ch = EMPTY;
				newf = calloc(1, sizeof(nfa_frag));
				newf->start_state = news;
				newf->last_state = news2;
                push(*newf);
                break;
            default:
                newf = calloc(1, sizeof(nfa_frag));
                news = calloc(1, sizeof(state));
                news->ch = ch;
                newf->start_state = news;
                newf->last_state = news;
                push(*newf);
                break;
        }
    }
    f1 = pop();
    if (sp != stack)
        error("nfa stack didn't balance\n");
#undef push
#undef pop
	f1.last_state->out = &accept_state;
	f1.last_state = &accept_state;
    return f1.start_state;
}

void addstate(list *l, state *s)
{
    if (s == NULL)
        return;
    if (s->ch == SPLIT) {
        addstate(l, s->out);
        addstate(l, s->out1);
        return ;
    }
	if (s->ch == EMPTY) {
		addstate(l, s->out);
		return ;
	}
    l->state_arr[l->num++] = s;
}

int step(list *cur_list, list *next_list, char ch)
{
    int i, flag;
    state *s;

    flag = -1;
    next_list->num = 0;
    memset(next_list->state_arr, 0, sizeof(next_list->state_arr));
    for (i = 0; i<cur_list->num; i++) {
        s = cur_list->state_arr[i];
        if (s == NULL)
            continue;
        if (ch == s->ch) {
            flag = 1;
            addstate(next_list, s->out);
        }
    }
    return flag;
}

int match(const char *str, state *start)
{
    char ch;
    list *cur_list, *next_list, *t;

    cur_list = &g_list1;
    next_list = &g_list2;
	memset(cur_list, 0, sizeof(g_list1));
	memset(next_list, 0, sizeof(g_list1));
    addstate(cur_list, start);
    while (ch = *str++) {
        if (step(cur_list, next_list, ch) < 0)
            return -1;
        t = cur_list; cur_list = next_list;
        next_list = t;
    }
	if (cur_list->state_arr[0] == &accept_state)
		return 1;
	else 
		return -1;
}

int main(int argc, char *argv[])
{
    state *start;
    char pattern[STKSIZE], string[STKSIZE], *post;
    int ret, i;

	i = 2;
	if (argc < 3) 
		error("usage: ./nfa regex string ...\n");
	post = reg2post(argv[1]);
	printf("postfix regex: %s\n", post);
	start = post2nfa(post);
	while (argv[i]) {
		ret = match(argv[i++], start);
        if (ret>0) printf("match\n");
        else printf("don't match\n");
	}
}

void error(const char *s)
{
    printf("%s", s);
    exit(-1);
}
