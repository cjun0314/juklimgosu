/*
 * 죽림고수 (竹林高手) - 대나무숲에서 벌어지는 최대의 결전! [기본 버전]
 * (한게임 플래시 '죽림고수'를 콘솔 게임으로 옮긴 버전)
 *
 * [규칙]
 *  생존법칙 1. 방향키로 캐릭터(O)를 움직여 날아오는 화살 피하기!
 *  생존법칙 2. 화살이 날아오는 패턴을 예측해서 무조건 오래 버티기!
 *  tip! 화살촉만 조심하세요! 화살의 다른 부분은 부딪쳐도 OK!
 *  화살촉에 한 번이라도 맞으면 게임 오버! 최대한 오래 버티세요.
 *
 * [개발 환경] Windows + Visual Studio
 *   (scanf_s, conio.h, windows.h 를 사용하므로 Windows 에서만 동작합니다)
 */

#include <stdio.h>    // printf, scanf_s, getchar
#include <stdlib.h>   // rand, srand, exit, abs, system
#include <string.h>   // strlen
#include <time.h>     // time (랜덤 값을 매번 다르게 만들 때 사용)
#include <conio.h>    // _kbhit, _getch (키보드 입력 비우기)
#include <windows.h>  // Sleep, GetTickCount, GetAsyncKeyState (방향키 읽기), 커서 조작, 콘솔 창 크기

 /* ===== 게임 화면 크기 (원형 대나무숲) =====
  * 맵의 크기는 게임을 시작할 때 콘솔 창 크기에 맞춰 자동으로 정해집니다. (setup_map 함수)
  * 창을 크게(최대화) 할수록 맵도 커집니다.
  * 맵을 더 크게/작게 하고 싶으면 아래 MAX_RADIUS_Y 숫자만 바꾸세요. */
#define MAX_RADIUS_Y 20   // 맵 세로 반지름의 최대값 (20이면 맵이 세로 41줄, 가로 81칸)
#define MIN_RADIUS_Y 6    // 맵 세로 반지름의 최소값 (창이 아주 작아도 이만큼은 유지)

#define MAX_FIELD_H  (MAX_RADIUS_Y * 2 + 1)   // 맵 세로 최대 칸 수
#define MAX_FIELD_W  (MAX_RADIUS_Y * 4 + 1)   // 맵 가로 최대 칸 수

#define MAX_ARROWS 400  // 화면에 동시에 있을 수 있는 화살 수
#define TICK_MS    50   // 한 프레임(한 번 움직이는 시간) = 0.05초

  /* 화면 아래 안내 글 */
#define FOOTER_TEXT "[방향키] 이동   화살촉( > < ^ v * )만 조심! 몸통은 OK"

/* 현재 맵 크기 (setup_map 에서 정해짐. 아래 값은 기본값) */
int field_w = 41;    // 맵 가로 칸 수
int field_h = 21;    // 맵 세로 칸 수
int center_x = 20;   // 원의 가운데 x
int center_y = 10;   // 원의 가운데 y
int radius_x = 20;   // 원의 가로 반지름
int radius_y = 10;   // 원의 세로 반지름 (글자 칸은 세로로 길어서 가로의 절반)

/* 콘솔 창 정보 (update_window_size 에서 읽어옴) */
int win_left = 0;    // 보이는 창의 왼쪽 끝 칸 번호
int win_top = 0;     // 보이는 창의 위쪽 끝 줄 번호
int win_w = 120;     // 창의 가로 칸 수
int win_h = 30;      // 창의 세로 줄 수

/* 게임 화면(제목줄 + 맵 + 안내줄)이 그려질 위치 */
int view_x = 0;      // 제목줄과 맵의 왼쪽 위 가로 위치
int view_y = 0;      // 제목줄의 세로 위치
int footer_x = 0;    // 안내줄의 가로 위치

/* 시작 화면에서 입력 안내("게임시작!...")를 쓸 위치 */
int rules_prompt_x = 0;
int rules_prompt_y = 0;

/* ===== 나(캐릭터) 위치 ===== */
int player_x = 20;
int player_y = 10;

/* ===== 화살 정보 (화살 번호 i 마다 아래 값이 하나씩 있음) =====
 * arrow_x, arrow_y : 화살촉 위치
 * arrow_dx, arrow_dy : 날아가는 방향 (-1, 0, 1)
 * arrow_alive : 1이면 날아가는 중, 0이면 없는 화살 */
int arrow_x[MAX_ARROWS];
int arrow_y[MAX_ARROWS];
int arrow_dx[MAX_ARROWS];
int arrow_dy[MAX_ARROWS];
int arrow_alive[MAX_ARROWS];

/* 화살이 나타나는 8개의 자리 (맵 크기에 맞춰 setup_map 에서 정해짐) */
int spawn_x[8];
int spawn_y[8];

char screen[MAX_FIELD_H][MAX_FIELD_W + 1];   // 한 프레임 화면 (글자 판)
double best_time = 0.0;                      // 최고 생존 기록(초)


/* ===== 숫자 입력 받는 함수 =====
 * 숫자를 입력하면 그 숫자를 돌려주고,
 * 글자를 잘못 입력하면 -1을 돌려줍니다. */
int get_number(void)
{
    int num = 0;
    int result = 0;
    int ch = 0;

    result = scanf_s("%d", &num);   // scanf 대신 scanf_s 사용

    if (result == EOF)              // 더 이상 입력을 받을 수 없을 때
    {
        printf("\n입력이 끝났습니다. 게임을 종료합니다.\n");
        exit(0);
    }

    // 입력창에 남은 글자를 줄 끝까지 비웁니다
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }

    if (result != 1)                // 숫자가 아닌 것을 입력함
    {
        return -1;
    }
    return num;
}


/* ===== 콘솔 화면 조작 함수들 ===== */

// 커서를 (x, y) 칸으로 옮기기 (화면을 지우지 않고 덮어써서 깜빡임을 줄입니다)
void gotoxy(int x, int y)
{
    COORD pos;

    fflush(stdout);   // 이전에 출력한 글자를 먼저 화면에 내보낸 뒤 커서를 옮김
    pos.X = (SHORT)x;
    pos.Y = (SHORT)y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// 커서(깜빡이는 막대)를 보이게(1) / 숨기게(0) 하기
void set_cursor_visible(int visible)
{
    CONSOLE_CURSOR_INFO info;

    info.dwSize = 20;
    if (visible == 1)
    {
        info.bVisible = TRUE;
    }
    else
    {
        info.bVisible = FALSE;
    }
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

// 키보드 입력창에 쌓인 키를 모두 버리기 (방향키가 scanf_s 입력에 섞이는 것을 막음)
void clear_key_buffer(void)
{
    while (_kbhit())
    {
        _getch();
    }
}


/* ===== 화면을 가운데에 놓기 위한 함수들 ===== */

// 지금 콘솔 창의 위치와 크기를 알아오기
void update_window_size(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) != 0)
    {
        win_left = info.srWindow.Left;
        win_top = info.srWindow.Top;
        win_w = info.srWindow.Right - info.srWindow.Left + 1;
        win_h = info.srWindow.Bottom - info.srWindow.Top + 1;
    }
}

// 가로 폭이 width 칸인 내용을 창 가운데에 놓으려면 몇 번째 칸에서 시작해야 하는지
int center_col(int width)
{
    int x = win_left + (win_w - width) / 2;

    if (x < win_left)
    {
        x = win_left;
    }
    return x;
}

// 세로 높이가 height 줄인 내용을 창 가운데에 놓으려면 몇 번째 줄에서 시작해야 하는지
int center_row(int height)
{
    int y = win_top + (win_h - height) / 2;

    if (y < win_top)
    {
        y = win_top;
    }
    return y;
}

// 화면의 y 번째 줄을 공백으로 지우기
void clear_row(int y)
{
    int i;

    gotoxy(win_left, y);
    for (i = 0; i < win_w - 1; i++)
    {
        putchar(' ');
    }
}

// 글자 text 가 화면에서 몇 칸을 차지하는지 재기
// 창 맨 윗줄에 실제로 출력해 보고, 커서가 몇 칸 움직였는지 확인한 뒤 바로 지웁니다.
// (한글은 2칸, 영어는 1칸이지만 특수 기호는 환경마다 달라서 직접 재는 것이 정확합니다)
int measure_width(const char* text)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    int width = (int)strlen(text);   // 재기에 실패하면 글자 바이트 수를 대신 사용
    int y;
    int last_y = win_top;

    gotoxy(win_left, win_top);
    printf("%s", text);
    fflush(stdout);

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) != 0)
    {
        last_y = info.dwCursorPosition.Y;
        // 줄바꿈 없이 한 줄에 들어갔을 때만 측정값을 믿음
        if (last_y == win_top && info.dwCursorPosition.X > win_left)
        {
            width = info.dwCursorPosition.X - win_left;
        }
    }

    for (y = win_top; y <= last_y; y++)   // 측정하려고 쓴 글자 지우기
    {
        clear_row(y);
    }
    return width;
}

// 창 크기에 맞춰 맵 크기와 화살이 나오는 자리, 화면 위치를 정하기
void setup_map(void)
{
    int ry;
    int rx_limit;

    update_window_size();

    // 세로: 제목줄 1 + 안내줄 1 + 위아래 여유 줄을 빼고 남는 줄에 맞춤
    ry = (win_h - 4 - 1) / 2;

    // 가로: 양옆 여유 칸을 빼고 맞춤 (가로 반지름은 세로 반지름의 2배)
    rx_limit = (win_w - 4 - 1) / 2;
    if (ry * 2 > rx_limit)
    {
        ry = rx_limit / 2;
    }

    if (ry > MAX_RADIUS_Y)
    {
        ry = MAX_RADIUS_Y;
    }
    if (ry < MIN_RADIUS_Y)
    {
        ry = MIN_RADIUS_Y;
    }

    radius_y = ry;
    radius_x = ry * 2;
    center_x = radius_x;
    center_y = radius_y;
    field_w = radius_x * 2 + 1;
    field_h = radius_y * 2 + 1;

    // 화살이 나타나는 8개의 자리 (왼쪽, 왼쪽 위, 위, 오른쪽 위, 오른쪽, 오른쪽 아래, 아래, 왼쪽 아래)
    spawn_x[0] = center_x - (radius_x - 2);       spawn_y[0] = center_y;
    spawn_x[1] = center_x - radius_x * 13 / 20;   spawn_y[1] = center_y - radius_y * 6 / 10;
    spawn_x[2] = center_x;                        spawn_y[2] = center_y - (radius_y - 2);
    spawn_x[3] = center_x + radius_x * 13 / 20;   spawn_y[3] = center_y - radius_y * 6 / 10;
    spawn_x[4] = center_x + (radius_x - 2);       spawn_y[4] = center_y;
    spawn_x[5] = center_x + radius_x * 13 / 20;   spawn_y[5] = center_y + radius_y * 6 / 10;
    spawn_x[6] = center_x;                        spawn_y[6] = center_y + (radius_y - 2);
    spawn_x[7] = center_x - radius_x * 13 / 20;   spawn_y[7] = center_y + radius_y * 6 / 10;

    // 게임 화면(제목줄 1 + 맵 + 안내줄 1)을 창 정가운데에 놓기
    view_x = center_col(field_w);
    view_y = center_row(field_h + 2);

    // 안내줄은 글자 폭을 재서 따로 가운데에 놓기
    footer_x = center_col(measure_width(FOOTER_TEXT));
}

// 입력 안내 글(message)을 (x, y) 칸에 쓰고 숫자를 입력받기
int ask_number(int x, int y, const char* message)
{
    // 그 줄에 남아 있는 예전 글자를 먼저 지움
    clear_row(y);

    gotoxy(x, y);
    printf("%s", message);
    fflush(stdout);
    return get_number();
}


/* ===== 원형 대나무숲 판별 함수들 ===== */

// (x, y) 칸이 타원(원) 안쪽에 있으면 1
int is_inside(int x, int y)
{
    double dx = (double)(x - center_x) / radius_x;
    double dy = (double)(y - center_y) / radius_y;

    if (dx * dx + dy * dy <= 1.0)
    {
        return 1;
    }
    return 0;
}

// (x, y) 칸이 원의 테두리(대나무 울타리)면 1
int is_border(int x, int y)
{
    if (is_inside(x, y) == 0)
    {
        return 0;
    }
    if (is_inside(x - 1, y) == 0 || is_inside(x + 1, y) == 0 ||
        is_inside(x, y - 1) == 0 || is_inside(x, y + 1) == 0)
    {
        return 1;
    }
    return 0;
}

// (x, y) 칸이 테두리 안쪽의 움직일 수 있는 칸이면 1
int is_playable(int x, int y)
{
    if (is_inside(x, y) == 1 && is_border(x, y) == 0)
    {
        return 1;
    }
    return 0;
}


/* ===== 화살 만들기 ===== */

// 양수면 1, 음수면 -1, 0이면 0
int get_sign(int n)
{
    if (n > 0)
    {
        return 1;
    }
    if (n < 0)
    {
        return -1;
    }
    return 0;
}

// (x, y) 에서 (dx, dy) 방향으로 날아가는 화살 하나 추가
void add_arrow(int x, int y, int dx, int dy)
{
    int i;

    if (dx == 0 && dy == 0)
    {
        return;
    }
    if (is_playable(x, y) == 0)   // 원 밖에는 만들지 않음
    {
        return;
    }
    for (i = 0; i < MAX_ARROWS; i++)
    {
        if (arrow_alive[i] == 0)   // 비어 있는 자리를 찾아서 저장
        {
            arrow_x[i] = x;
            arrow_y[i] = y;
            arrow_dx[i] = dx;
            arrow_dy[i] = dy;
            arrow_alive[i] = 1;
            break;
        }
    }
}

// (x, y) 에서 목표 (target_x, target_y) 쪽으로 날아가는 화살 추가
// 8방향(상하좌우 + 대각선) 중 가장 가까운 방향을 고릅니다.
void add_arrow_toward(int x, int y, int target_x, int target_y)
{
    int gap_x = target_x - x;
    int gap_y = (target_y - y) * 2;   // 세로 한 칸은 가로 두 칸 정도의 길이라 2배로 계산
    int dx = 0;
    int dy = 0;

    if (abs(gap_x) * 2 > abs(gap_y))
    {
        dx = get_sign(gap_x);   // 가로로 충분히 멀면 좌우로 움직임
    }
    if (abs(gap_y) * 2 > abs(gap_x))
    {
        dy = get_sign(gap_y);   // 세로로 충분히 멀면 위아래로 움직임
    }
    add_arrow(x, y, dx, dy);
}


/* ===== 화살이 날아오는 패턴 4가지 ===== */

// 패턴 1: 원 둘레 8곳에서 한가운데를 향해 동시에 날아옴
void pattern_ring(void)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        add_arrow_toward(spawn_x[i], spawn_y[i], center_x, center_y);
    }
}

// 패턴 2: 3곳에서 지금 내 위치를 노리고 날아옴
void pattern_aimed(void)
{
    int i;
    int n;

    for (i = 0; i < 3; i++)
    {
        n = rand() % 8;
        add_arrow_toward(spawn_x[n], spawn_y[n], player_x, player_y);
    }
}

// 패턴 3: 가로줄 화살이 한쪽에서 몰려옴 (빈 줄 하나가 안전지대!)
void pattern_rows(int gap_half)
{
    int x;
    int y;
    int span = radius_y * 6 / 10;                           // 안전지대가 나올 수 있는 범위
    int gap_y = center_y - span + rand() % (span * 2 + 1);  // 비워둘 줄의 가운데
    int from_left = rand() % 2;                             // 1이면 왼쪽에서, 0이면 오른쪽에서

    for (y = center_y - (radius_y - 2); y <= center_y + (radius_y - 2); y++)
    {
        if (y >= gap_y - gap_half && y <= gap_y + gap_half)
        {
            continue;   // 안전지대 줄은 화살을 만들지 않음
        }

        if (from_left == 1)
        {
            x = 0;
            while (x < field_w && is_playable(x, y) == 0)   // 그 줄의 맨 왼쪽 칸 찾기
            {
                x++;
            }
            add_arrow(x, y, 1, 0);
        }
        else
        {
            x = field_w - 1;
            while (x >= 0 && is_playable(x, y) == 0)        // 그 줄의 맨 오른쪽 칸 찾기
            {
                x--;
            }
            add_arrow(x, y, -1, 0);
        }
    }
}

// 패턴 4: 세로줄 화살이 위(또는 아래)에서 쏟아짐 (빈 곳이 안전지대!)
void pattern_cols(int gap_half)
{
    int x;
    int y;
    int span = radius_x * 12 / 20;                          // 안전지대가 나올 수 있는 범위
    int gap_x = center_x - span + rand() % (span * 2 + 1);  // 비워둘 칸의 가운데
    int from_top = rand() % 2;                              // 1이면 위에서, 0이면 아래에서

    for (x = center_x - (radius_x - 1); x <= center_x + (radius_x - 1); x++)
    {
        if (x >= gap_x - gap_half && x <= gap_x + gap_half)
        {
            continue;   // 안전지대 칸은 화살을 만들지 않음
        }

        if (from_top == 1)
        {
            y = 0;
            while (y < field_h && is_playable(x, y) == 0)   // 그 칸의 맨 위 찾기
            {
                y++;
            }
            add_arrow(x, y, 0, 1);
        }
        else
        {
            y = field_h - 1;
            while (y >= 0 && is_playable(x, y) == 0)        // 그 칸의 맨 아래 찾기
            {
                y--;
            }
            add_arrow(x, y, 0, -1);
        }
    }
}

// 패턴 하나를 랜덤으로 골라서 실행 (오래 버틸수록 안전지대가 좁아짐)
void spawn_pattern(double elapsed)
{
    int kind = rand() % 4;
    int gap_half = 2;

    if (elapsed >= 15.0)
    {
        gap_half = 1;
    }

    if (kind == 0)
    {
        pattern_ring();
    }
    else if (kind == 1)
    {
        pattern_aimed();
    }
    else if (kind == 2)
    {
        pattern_rows(gap_half);
    }
    else
    {
        pattern_cols(gap_half + 1);
    }
}


/* ===== 난이도: 시간이 지날수록 어려워짐 ===== */

// 몇 프레임마다 새 패턴이 나오는지 (처음 40프레임 = 2초, 점점 짧아져서 최소 14프레임)
int get_spawn_interval(double elapsed)
{
    int interval = 40 - (int)elapsed;

    if (interval < 14)
    {
        interval = 14;
    }
    return interval;
}

// 화살이 몇 프레임마다 한 번 움직이는지 (25초가 지나면 2배 빨라짐)
int get_arrow_delay(double elapsed)
{
    if (elapsed >= 25.0)
    {
        return 1;
    }
    return 2;
}


/* ===== 움직이기 ===== */

// 방향키를 읽어서 내 캐릭터 움직이기
void move_player(int tick)
{
    int new_x = player_x;
    int new_y = player_y;

    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
    {
        new_x--;
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
    {
        new_x++;
    }

    // 글자 칸은 세로로 길어서, 위아래는 2프레임에 한 칸씩 움직입니다
    if (tick % 2 == 0)
    {
        if (GetAsyncKeyState(VK_UP) & 0x8000)
        {
            new_y--;
        }
        if (GetAsyncKeyState(VK_DOWN) & 0x8000)
        {
            new_y++;
        }
    }

    // 대나무 울타리 안쪽일 때만 움직임
    if (is_playable(new_x, player_y) == 1)
    {
        player_x = new_x;
    }
    if (is_playable(player_x, new_y) == 1)
    {
        player_y = new_y;
    }
}

// 날아가는 모든 화살을 한 칸 움직이기
void move_arrows(void)
{
    int i;

    for (i = 0; i < MAX_ARROWS; i++)
    {
        if (arrow_alive[i] == 1)
        {
            arrow_x[i] += arrow_dx[i] * 2;   // 가로는 2칸씩 (글자 칸이 세로로 길어서)
            arrow_y[i] += arrow_dy[i];

            if (is_playable(arrow_x[i], arrow_y[i]) == 0)   // 원 밖으로 나가면 사라짐
            {
                arrow_alive[i] = 0;
            }
        }
    }
}

// 화살촉이 나에게 맞았으면 1 (화살 몸통은 맞아도 괜찮음!)
int is_hit(void)
{
    int i;
    int gap;

    for (i = 0; i < MAX_ARROWS; i++)
    {
        if (arrow_alive[i] == 1 && arrow_y[i] == player_y)
        {
            gap = arrow_x[i] - player_x;

            if (arrow_dx[i] == 0)   // 위아래로 날아오는 화살은 같은 칸일 때만
            {
                if (gap == 0)
                {
                    return 1;
                }
            }
            else                    // 옆으로 날아오는 화살은 2칸씩 움직이므로 양옆 1칸까지
            {
                if (gap >= -1 && gap <= 1)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}


/* ===== 화면 그리기 ===== */

// 화살촉 모양 글자
char head_char(int dx, int dy)
{
    if (dy == 0)
    {
        if (dx > 0)
        {
            return '>';
        }
        return '<';
    }
    if (dx == 0)
    {
        if (dy > 0)
        {
            return 'v';
        }
        return '^';
    }
    return '*';   // 대각선 화살촉
}

// 화살 몸통 모양 글자
char tail_char(int dx, int dy)
{
    if (dy == 0)
    {
        return '-';
    }
    if (dx == 0)
    {
        return '|';
    }
    if (dx == dy)
    {
        return '\\';
    }
    return '/';
}

// 한 프레임을 화면에 그리기
void draw_game(double elapsed)
{
    int x;
    int y;
    int i;
    int k;
    int unit_x;
    int tx;
    int ty;
    int time_x;

    // 1) 빈 화면 만들기 (원의 테두리는 '#')
    for (y = 0; y < field_h; y++)
    {
        for (x = 0; x < field_w; x++)
        {
            if (is_border(x, y) == 1)
            {
                screen[y][x] = '#';
            }
            else
            {
                screen[y][x] = ' ';
            }
        }
        screen[y][field_w] = '\0';   // 글자 끝 표시
    }

    // 2) 화살 몸통 그리기 (화살촉 뒤쪽 2칸)
    for (i = 0; i < MAX_ARROWS; i++)
    {
        if (arrow_alive[i] == 1)
        {
            unit_x = 2;
            if (arrow_dy[i] == 0)
            {
                unit_x = 1;
            }

            for (k = 1; k <= 2; k++)
            {
                tx = arrow_x[i] - arrow_dx[i] * unit_x * k;
                ty = arrow_y[i] - arrow_dy[i] * k;
                if (is_playable(tx, ty) == 1)
                {
                    screen[ty][tx] = tail_char(arrow_dx[i], arrow_dy[i]);
                }
            }
        }
    }

    // 3) 화살촉 그리기 (몸통 위에 덮어씀)
    for (i = 0; i < MAX_ARROWS; i++)
    {
        if (arrow_alive[i] == 1)
        {
            screen[arrow_y[i]][arrow_x[i]] = head_char(arrow_dx[i], arrow_dy[i]);
        }
    }

    // 4) 나 그리기
    screen[player_y][player_x] = 'O';

    // 5) 화면에 출력 (창 가운데 위치에, 한 줄씩 커서를 옮겨가며 그림)
    //    제목줄: 왼쪽에는 이름, 오른쪽 끝에는 생존 시간
    gotoxy(view_x, view_y);
    printf("  죽림고수");

    time_x = view_x + field_w - 19;      // 생존 시간 글자(19칸)를 맵 오른쪽 끝에 맞춤
    if (time_x < view_x + 12)
    {
        time_x = view_x + 12;
    }
    gotoxy(time_x, view_y);
    printf("생존 시간: %6.2f초", elapsed);

    //    맵
    for (y = 0; y < field_h; y++)
    {
        gotoxy(view_x, view_y + 1 + y);
        printf("%s", screen[y]);
    }

    //    안내줄
    gotoxy(footer_x, view_y + 1 + field_h);
    printf("%s", FOOTER_TEXT);
    fflush(stdout);
}


/* ===== 게임 한 판 =====
 * 화살에 맞을 때까지 진행하고, 버틴 시간(초)을 돌려줍니다. */
double play_game(void)
{
    int i;
    int tick = 0;                // 지금까지 지난 프레임 수
    int spawn_timer = 20;        // 다음 패턴이 나올 때까지 남은 프레임 (처음엔 1초 뒤)
    int hit = 0;                 // 맞았으면 1
    double elapsed = 0.0;        // 버틴 시간(초)
    DWORD start_tick = 0;        // 게임을 시작한 시각

    // 게임 준비: 화살 초기화
    for (i = 0; i < MAX_ARROWS; i++)
    {
        arrow_alive[i] = 0;
    }

    system("cls");
    setup_map();                 // 창 크기에 맞춰 맵 크기와 화면 위치를 정함
    player_x = center_x;         // 나는 맵 한가운데에서 시작
    player_y = center_y;
    set_cursor_visible(0);
    clear_key_buffer();
    start_tick = GetTickCount();

    while (hit == 0)
    {
        elapsed = (double)(GetTickCount() - start_tick) / 1000.0;

        // 1) 내가 움직임
        move_player(tick);
        if (is_hit() == 1)
        {
            hit = 1;
        }

        // 2) 화살이 날아감
        if (hit == 0 && tick % get_arrow_delay(elapsed) == 0)
        {
            move_arrows();
            if (is_hit() == 1)
            {
                hit = 1;
            }
        }

        // 3) 새 화살 패턴 등장
        spawn_timer--;
        if (spawn_timer <= 0)
        {
            spawn_pattern(elapsed);
            spawn_timer = get_spawn_interval(elapsed);
        }

        // 4) 화면 그리기
        draw_game(elapsed);
        clear_key_buffer();
        Sleep(TICK_MS);          // 5) 잠깐 쉬기
        tick++;
    }

    Sleep(700);                  // 맞은 장면을 잠깐 보여주기
    clear_key_buffer();
    set_cursor_visible(1);
    return elapsed;
}


/* ===== 시작 화면 (줄마다 따로 좌표를 계산해서 창 가운데에 표시) =====
 * 줄마다 길이가 달라서 한꺼번에 가운데로 놓으면 왼쪽으로 쏠려 보입니다.
 * 그래서 각 줄의 글자 폭을 재고, 그 줄만의 가로 좌표를 따로 구해서 그립니다. */
#define RULES_LINES  17   // 규칙 글의 줄 수
#define START_PROMPT "게임시작! (1을 입력하고 Enter) >> "

void show_rules(void)
{
    const char* rules[RULES_LINES] = {
        "========================================================",
        "죽  림  고  수",
        "========================================================",
        "대나무숲에서 벌어지는 최대의 결전!",
        "",
        "죽림고수는 냉정한 무림세계에서",
        "누가 오래 살아남을 수 있는지 경쟁하는 게임입니다.",
        "",
        "생존법칙 1. 방향키로 캐릭터(O)를 움직여 화살 피하기!",
        "",
        "생존법칙 2. 화살 패턴을 예측하고 무조건 오래 버티기!",
        "",
        "tip! 화살을 피할 때는 화살촉만 조심하세요!",
        "화살의 다른 부분은 부딪쳐도 OK!",
        "",
        "험난한 경쟁을 극복하고 진정한 죽림고수가 되어보세요!",
        "========================================================"
    };
    int line_x[RULES_LINES];   // 각 줄의 가로 시작 좌표
    int i;
    int y;

    system("cls");
    update_window_size();

    // 1) 가로 좌표: 줄마다 폭을 재서, 그 줄이 가운데에 오는 좌표를 따로 구함
    for (i = 0; i < RULES_LINES; i++)
    {
        if (rules[i][0] == '\0')
        {
            line_x[i] = win_left;   // 빈 줄은 그릴 것이 없음
        }
        else
        {
            line_x[i] = center_col(measure_width(rules[i]));
        }
    }
    rules_prompt_x = center_col(measure_width(START_PROMPT));

    // 2) 세로 좌표: (규칙 + 빈 줄 + 입력 안내 줄) 전체가 창 한가운데에 오도록 시작 줄을 구함
    y = center_row(RULES_LINES + 2);

    // 3) 줄마다 (가로 좌표, 세로 좌표)를 지정해서 그리기
    for (i = 0; i < RULES_LINES; i++)
    {
        if (rules[i][0] != '\0')
        {
            gotoxy(line_x[i], y + i);
            printf("%s", rules[i]);
        }
    }
    rules_prompt_y = y + RULES_LINES + 1;
}


#define RETRY_PROMPT "다시 도전하시겠습니까? (1: 예, 2: 아니오) >> "

int main(void)
{
    int ready = 0;        // 시작 입력용
    int play_again = 1;   // 1이면 한 판 더
    int answer = 0;       // 다시 할지 묻는 입력용
    int x = 0;
    int y = 0;
    int row = 0;
    int retry_x = 0;      // 다시 하기 질문의 가로 위치
    double elapsed = 0.0; // 이번 판에 버틴 시간

    system("color 0A");   // 초록색 글자(검정 바탕)로 바꾸기
    srand((unsigned int)time(NULL));   // 실행할 때마다 다른 랜덤 값을 만들기

    show_rules();

    // 1을 입력할 때까지 기다립니다
    while (ready != 1)
    {
        ready = ask_number(rules_prompt_x, rules_prompt_y, START_PROMPT);
    }

    while (play_again == 1)
    {
        elapsed = play_game();

        // 게임 오버 화면 (창 가운데에 표시)
        system("cls");
        update_window_size();
        retry_x = center_col(measure_width(RETRY_PROMPT));
        x = center_col(30);
        y = center_row(10);

        gotoxy(x, y);
        printf("========== GAME OVER ==========");
        gotoxy(x, y + 2);
        printf("        (ㅠㅅㅠ) 엉엉...");

        row = y + 4;
        gotoxy(x, row);
        printf(" 이번 생존 시간 : %.2f초", elapsed);
        row++;

        if (elapsed > best_time)
        {
            best_time = elapsed;
            gotoxy(x, row);
            printf(" 새로운 최고 기록입니다!");
            row++;
        }
        gotoxy(x, row);
        printf(" 최고 기록      : %.2f초", best_time);
        row += 2;

        // 1(다시 하기) 또는 2(끝내기)를 입력할 때까지 묻습니다
        answer = 0;
        while (answer != 1 && answer != 2)
        {
            answer = ask_number(retry_x, row, RETRY_PROMPT);
        }

        if (answer == 2)
        {
            play_again = 0;
        }
    }

    gotoxy(center_col(46), row + 2);
    printf("진정한 죽림고수를 향한 도전, 다음에 또 만나요!");
    gotoxy(win_left, row + 4);
    return 0;
}