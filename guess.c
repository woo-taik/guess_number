/*
 * 숫자 맞추기 게임
 *
 * 컴퓨터가 1~100 사이의 난수를 생성하면 사용자가 그 숫자를 맞춘다.
 * 입력할 때마다 정답보다 큰지 작은지 알려주고, 맞추면 한 판이 끝난다.
 * 판이 끝나면 어떤 숫자를 어떤 과정으로 몇 번 만에 맞췄는지 출력한 뒤,
 * 게임을 계속할지 사용자에게 묻는다.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#  include <windows.h>   
#  include <process.h>   
#  define GET_PID() _getpid()
#else
#  include <unistd.h>    /* getpid */
#  define GET_PID() getpid()
#endif

#define LINE_BUF_SIZE 128
#define NUMBER_MIN 1
#define NUMBER_MAX 100
#define MAX_ATTEMPTS 15 

typedef struct {
    const char *name;
    int max_attempts;
} Difficulty;

static const Difficulty DIFFICULTIES[] = {
    { "쉬움",   MAX_ATTEMPTS },
    { "보통",   10 },
    { "어려움",  5 },
};

#define DIFFICULTY_COUNT ((int)(sizeof DIFFICULTIES / sizeof DIFFICULTIES[0]))

/* 한 줄을 읽어 개행을 제거한다. EOF면 0, 성공하면 1을 반환한다. */
static int read_line(char *buf, size_t size)
{
    size_t len;

    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
    return 1;
}

/* 문자열 전체가 하나의 정수일 때만 파싱에 성공한다. */
static int parse_int(const char *s, int *out)
{
    char *end;
    long value;

    while (isspace((unsigned char)*s))
        s++;
    if (*s == '\0')
        return 0;

    errno = 0;
    value = strtol(s, &end, 10);
    if (end == s || errno == ERANGE || value < INT_MIN || value > INT_MAX)
        return 0;

    while (isspace((unsigned char)*end))
        end++;
    if (*end != '\0')
        return 0;

    *out = (int)value;
    return 1;
}

/* min~max 범위의 정수를 받을 때까지 되묻는다. EOF면 0을 반환한다. */
static int read_int_in_range(const char *prompt, int min, int max, int *out)
{
    char line[LINE_BUF_SIZE];
    int value;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!read_line(line, sizeof line))
            return 0;

        if (!parse_int(line, &value)) {
            printf("숫자를 입력해주세요.\n");
            continue;
        }
        if (value < min || value > max) {
            printf("%d~%d 사이로 입력해주세요.\n", min, max);
            continue;
        }

        *out = value;
        return 1;
    }
}

/* y면 1, n이면 0. EOF도 0(종료)으로 취급한다. */
static int ask_yes_no(const char *prompt)
{
    char line[LINE_BUF_SIZE];
    const char *p;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!read_line(line, sizeof line))
            return 0;

        for (p = line; isspace((unsigned char)*p); p++)
            ;

        if (*p == 'y' || *p == 'Y')
            return 1;
        if (*p == 'n' || *p == 'N')
            return 0;

        printf("y 또는 n을 입력해주세요.\n");
    }
}

/* 난이도를 고른다. EOF면 NULL을 반환한다. */
static const Difficulty *select_difficulty(void)
{
    int i, choice;

    printf("\n=== 난이도 선택 ===\n");
    for (i = 0; i < DIFFICULTY_COUNT; i++)
        printf("%d) %s - %d번의 기회\n", i + 1, DIFFICULTIES[i].name, DIFFICULTIES[i].max_attempts);

    if (!read_int_in_range("선택 (1-3): ", 1, DIFFICULTY_COUNT, &choice))
        return NULL;

    return &DIFFICULTIES[choice - 1];
}

/* 한 판이 끝난 뒤 정답, 시도 횟수, 시도 과정을 한꺼번에 보여준다. */
static void print_result(const Difficulty *difficulty, int answer,
                         const int *history, int count, int won)
{
    int i;

    printf("\n========= 게임 결과 =========\n");
    printf("난이도   : %s (%d번의 기회)\n", difficulty->name, difficulty->max_attempts);
    printf("정답     : %d\n", answer);
    if (won)
        printf("결과     : 성공! %d번 만에 맞췄습니다.\n", count);
    else
        printf("결과     : 실패 (%d번의 기회를 모두 사용)\n", count);

    printf("진행 과정 :\n");
    for (i = 0; i < count; i++) {
        const char *hint;

        if (history[i] == answer)
            hint = "정답!";
        else if (history[i] < answer)
            hint = "더 큽니다";
        else
            hint = "더 작습니다";

        printf("  %2d번째 시도: %3d  ->  %s\n", i + 1, history[i], hint);
    }
    printf("=============================\n");
}

/* 한 판을 진행한다. 도중에 EOF를 만나면 0을 반환한다. */
static int play_round(const Difficulty *difficulty)
{
    const int answer = rand() % (NUMBER_MAX - NUMBER_MIN + 1) + NUMBER_MIN;
    int history[MAX_ATTEMPTS];   /* 사용자가 시도한 숫자를 순서대로 기록 */
    int count = 0;               /* 지금까지의 시도 횟수 */
    int remaining = difficulty->max_attempts;
    int won = 0;
    int guess;
    char prompt[64];

    snprintf(prompt, sizeof prompt, "숫자를 입력하세요 (%d-%d): ", NUMBER_MIN, NUMBER_MAX);

    printf("\n[%s] %d~%d 사이의 숫자를 %d번 안에 맞춰보세요!\n",
           difficulty->name, NUMBER_MIN, NUMBER_MAX, difficulty->max_attempts);

    while (remaining > 0) {
        if (!read_int_in_range(prompt, NUMBER_MIN, NUMBER_MAX, &guess))
            return 0;

        history[count] = guess;   /* 시도 과정 기록 */
        count++;
        remaining--;

        if (guess == answer) {
            won = 1;
            break;
        }

        printf("%s (남은 기회: %d)\n",
               guess < answer ? "더 큽니다!" : "더 작습니다!", remaining);
    }

    print_result(difficulty, answer, history, count, won);
    return 1;
}

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   /* Windows 콘솔에서 한글이 깨지지 않도록 */
#endif

    srand((unsigned int)time(NULL) ^ ((unsigned int)GET_PID() << 16));

    printf("=== 숫자 맞추기 게임 ===\n");

    for (;;) {
        const Difficulty *difficulty = select_difficulty();
        if (difficulty == NULL)
            break;

        if (!play_round(difficulty))
            break;

        if (!ask_yes_no("\n새 게임을 시작하시겠습니까? (y/n): "))
            break;
    }

    printf("\n게임을 종료합니다. 안녕히 가세요!\n");
    return 0;
}
