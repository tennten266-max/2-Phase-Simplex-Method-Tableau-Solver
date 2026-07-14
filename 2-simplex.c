#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- 分数（有理数）構造体と演算 ---
typedef struct {
    long long num; // 分子
    long long den; // 分母
} Rational;

long long gcd(long long a, long long b) {
    while (b != 0) {
        long long t = b;
        b = a % b;
        a = t;
    }
    return a < 0 ? -a : a;
}

Rational make_rational(long long n, long long d) {
    if (d < 0) { n = -n; d = -d; }
    long long g = gcd(n, d);
    return (Rational){n / g, d / g};
}

Rational r_add(Rational a, Rational b) { return make_rational(a.num * b.den + b.num * a.den, a.den * b.den); }
Rational r_sub(Rational a, Rational b) { return make_rational(a.num * b.den - b.num * a.den, a.den * b.den); }
Rational r_mul(Rational a, Rational b) { return make_rational(a.num * b.num, a.den * b.den); }
Rational r_div(Rational a, Rational b) { return make_rational(a.num * b.den, a.den * b.num); }
Rational r_neg(Rational a) { return make_rational(-a.num, a.den); }

// 分数を文字列に変換（幅の計測用）
void format_rational(Rational r, char *buf) {
    if (r.den == 1) {
        sprintf(buf, "%lld", r.num);
    } else {
        sprintf(buf, "%lld/%lld", r.num, r.den);
    }
}

// 変数のインデックスを名前に変換（0はxa、それ以外はx1, x2...）
void format_var_name(int idx, char *buf) {
    if (idx == 0) {
        strcpy(buf, "xa");
    } else {
        sprintf(buf, "x%d", idx);
    }
}

// --- 動的空白調整付きのタブロー表示関数 ---
void print_tableau(Rational **T, int *row_vars, int *col_vars, int num_rows, int num_cols, int has_za, Rational *row_z_orig, int phase) {
    int *col_widths = calloc(num_cols + 2, sizeof(int));
    char buf[64];

    // 1. 左端（変数名）列の幅設定
    col_widths[0] = 5; 
    for (int i = 0; i < num_rows; i++) {
        format_var_name(row_vars[i], buf);
        int len = strlen(buf);
        if (len > col_widths[0]) col_widths[0] = len;
    }

    // 2. 定数項列の幅設定
    col_widths[1] = 8; 
    for (int i = 0; i < num_rows; i++) {
        format_rational(T[i][0], buf);
        int len = strlen(buf);
        if (len > col_widths[1]) col_widths[1] = len;
    }
    if (has_za) {
        format_rational(row_z_orig[0], buf);
        int len = strlen(buf);
        if (len > col_widths[1]) col_widths[1] = len;
    }

    // 3. 各非基底変数列の幅設定
    for (int j = 0; j < num_cols; j++) {
        col_widths[j + 2] = 8; 
        format_var_name(col_vars[j], buf);
        int len_name = strlen(buf);
        if (len_name > col_widths[j + 2]) col_widths[j + 2] = len_name;

        for (int i = 0; i < num_rows; i++) {
            format_rational(T[i][j + 1], buf);
            int len = strlen(buf);
            if (len > col_widths[j + 2]) col_widths[j + 2] = len;
        }
        if (has_za) {
            format_rational(row_z_orig[j + 1], buf);
            int len = strlen(buf);
            if (len > col_widths[j + 2]) col_widths[j + 2] = len;
        }
    }

    // --- 描画処理 ---
    // ヘッダー行
    printf("%*s  %*s", col_widths[0], "◯", col_widths[1], "");
    for (int j = 0; j < num_cols; j++) {
        format_var_name(col_vars[j], buf);
        printf("  %*s", col_widths[j + 2], buf);
    }
    printf("\n");

    // Za 行 (第1段階のみ)
    if (has_za && phase == 1) {
        printf("%*s", col_widths[0], "Za");
        for (int j = 0; j <= num_cols; j++) {
            format_rational(T[0][j], buf);
            printf("  %*s", col_widths[j + 1], buf);
        }
        printf("\n");
    }

    // Z 行
    printf("%*s", col_widths[0], "Z");
    for (int j = 0; j <= num_cols; j++) {
        format_rational(has_za ? row_z_orig[j] : T[0][j], buf);
        printf("  %*s", col_widths[j + 1], buf);
    }
    printf("\n");

    // 各制約条件行
    for (int i = 1; i < num_rows; i++) {
        format_var_name(row_vars[i], buf);
        printf("%*s", col_widths[0], buf);
        for (int j = 0; j <= num_cols; j++) {
            format_rational(T[i][j], buf);
            printf("  %*s", col_widths[j + 1], buf);
        }
        printf("\n");
    }
    printf("\n");

    free(col_widths);
}

int main() {
    int m;
    if (scanf("%d", &m) != 1) return 0;

    int max_vars = 1000;
    Rational *input_c = malloc(sizeof(Rational) * max_vars);
    int n_orig = 0;
    while (n_orig < max_vars) {
        long long val;
        if (scanf("%lld", &val) != 1) break;
        input_c[n_orig++] = make_rational(val, 1);
        char ch = getchar();
        if (ch == '\n' || ch == EOF) break;
    }

    Rational **input_A = malloc(sizeof(Rational*) * m);
    for (int i = 0; i < m; i++) {
        input_A[i] = malloc(sizeof(Rational) * (n_orig + 1));
        for (int j = 0; j <= n_orig; j++) {
            long long val;
            if (scanf("%lld", &val) != 1) return 1;
            input_A[i][j] = make_rational(val, 1);
        }
    }

    // --- 初期タブローの組み立て ---
    int num_rows = 1 + m; 
    int num_cols = n_orig; 

    int *row_vars = malloc(sizeof(int) * num_rows);
    int *col_vars = malloc(sizeof(int) * (num_cols + 1)); 

    for (int j = 0; j < num_cols; j++) col_vars[j] = j + 1; 
    for (int i = 0; i < m; i++) row_vars[i + 1] = n_orig + 1 + i; 

    Rational **T = malloc(sizeof(Rational*) * num_rows);
    for (int i = 0; i < num_rows; i++) {
        T[i] = calloc(num_cols + 2, sizeof(Rational));
    }

    // 辞書形式への落とし込み: s = -b + Ax
    for (int i = 0; i < m; i++) {
        T[i + 1][0] = r_neg(input_A[i][n_orig]); 
        for (int j = 0; j < n_orig; j++) {
            T[i + 1][j + 1] = input_A[i][j]; 
        }
    }

    // 目的関数 Z 行の設定
    T[0][0] = make_rational(0, 1);
    for (int j = 0; j < n_orig; j++) {
        T[0][j + 1] = input_c[j];
    }

    // 負の定数項を持つ行の探索（第1段階の判定）
    int need_phase1 = 0;
    int min_b_row = -1;
    for (int i = 1; i <= m; i++) {
        if (T[i][0].num < 0) {
            need_phase1 = 1;
            if (min_b_row == -1 || r_sub(T[i][0], T[min_b_row][0]).num < 0) {
                min_b_row = i;
            }
        }
    }

    int phase = need_phase1 ? 1 : 2;
    int has_xa = 0;
    Rational *row_z_orig = NULL; 

    printf("初期状態\n");
    if (phase == 1) {
        has_xa = 1;
        num_cols++; 
        col_vars[num_cols - 1] = 0; // 人工変数 xa を 0 とする

        // 各制約条件に +1 * xa を導入
        for (int i = 1; i <= m; i++) {
            T[i][num_cols] = make_rational(1, 1);
        }

        // 本来の目的関数 Z の退避
        row_z_orig = calloc(num_cols + 1, sizeof(Rational));
        for (int j = 0; j < num_cols; j++) row_z_orig[j] = T[0][j];
        row_z_orig[num_cols] = make_rational(0, 1); 

        // 補助目的関数行 Za = 0 + 0*x1 + ... + 1*xa の設定
        for (int j = 0; j < num_cols; j++) T[0][j] = make_rational(0, 1);
        T[0][num_cols] = make_rational(1, 1);

        print_tableau(T, row_vars, col_vars, num_rows, num_cols, 1, row_z_orig, 1);

        // --- 人工変数 xa を基底に入れるための初期特殊ピボット演算 ---
        int leaving_row = min_b_row;
        int entering_col = num_cols - 1; 

        printf("初期ピボット演算 (出る変数: x%d, 入る変数: xa)\n\n", row_vars[leaving_row]);

        Rational pivot = T[leaving_row][entering_col + 1];
        
        // 出る行の係数更新
        for (int j = 0; j <= num_cols; j++) {
            if (j != entering_col + 1) {
                T[leaving_row][j] = r_neg(r_div(T[leaving_row][j], pivot));
            }
        }
        T[leaving_row][entering_col + 1] = r_div(make_rational(1, 1), pivot);

        // 他行の係数更新
        for (int i = 0; i < num_rows; i++) {
            if (i != leaving_row) {
                Rational factor = T[i][entering_col + 1];
                T[i][entering_col + 1] = make_rational(0, 1);
                for (int j = 0; j <= num_cols; j++) {
                    T[i][j] = r_add(T[i][j], r_mul(factor, T[leaving_row][j]));
                }
            }
        }
        // 退避している Z 行の更新
        Rational factor_z = row_z_orig[entering_col + 1];
        row_z_orig[entering_col + 1] = make_rational(0, 1);
        for (int j = 0; j <= num_cols; j++) {
            row_z_orig[j] = r_add(row_z_orig[j], r_mul(factor_z, T[leaving_row][j]));
        }

        // 変数の入れ替え
        int temp = row_vars[leaving_row];
        row_vars[leaving_row] = col_vars[entering_col];
        col_vars[entering_col] = temp;

        print_tableau(T, row_vars, col_vars, num_rows, num_cols, 1, row_z_orig, 1);
    } else {
        print_tableau(T, row_vars, col_vars, num_rows, num_cols, 0, NULL, 2);
    }

    // --- メイン単体法ループ ---
    int iter = 1;
    while (1) {
        // 1. 入る変数の選択 (Bland規則: 目的関数行の係数が「負(<0)」のもののうち、変数添字が最小のもの)
        // 最小化問題の辞書形式 z = v + cx において、c_j < 0 が改善可能な非基底変数
        int entering_col = -1;
        int min_entering_idx = -1;
        for (int j = 0; j < num_cols; j++) {
            if (T[0][j + 1].num < 0) {  // 【修正】 >0 から <0 へ変更
                int v_idx = col_vars[j];
                if (min_entering_idx == -1 || v_idx < min_entering_idx) {
                    min_entering_idx = v_idx;
                    entering_col = j;
                }
            }
        }

        // 改善候補（負の係数）がなければ、現在のフェーズを終了（最適に達した）
        if (entering_col == -1) {
            if (phase == 1) {
                if (T[0][0].num != 0) {
                    printf("補助問題の最適値が0でないため、元問題は実行不可能です。\n");
                    return 0;
                }
                
                phase = 2;
                printf("第2段階に移行します。\n\n");

                // 非基底から人工変数 xa (0) を探し出し、タブローの列から完全に除去
                int xa_idx = -1;
                for (int j = 0; j < num_cols; j++) {
                    if (col_vars[j] == 0) { xa_idx = j; break; }
                }

                if (xa_idx != -1) {
                    for (int i = 0; i < num_rows; i++) {
                        for (int j = xa_idx; j < num_cols - 1; j++) {
                            T[i][j + 1] = T[i][j + 2];
                        }
                    }
                    for (int j = xa_idx; j < num_cols - 1; j++) {
                        col_vars[j] = col_vars[j + 1];
                        row_z_orig[j + 1] = row_z_orig[j + 2];
                    }
                    num_cols--;
                }

                // T[0] 行に本来の目的関数 Z を書き戻す
                for (int j = 0; j <= num_cols; j++) T[0][j] = row_z_orig[j];
                has_xa = 0;

                print_tableau(T, row_vars, col_vars, num_rows, num_cols, 0, NULL, 2);
                continue;
            } else {
                printf("最適解に到達しました。\n");
                break;
            }
        }

        // 2. 出る変数の選択 (Bland規則: 係数が「負(<0)」である行のうち、-定数項/係数 が最小のもの)
        // 辞書形式 x_i = b_i + a_ij * x_j において、a_ij < 0 のとき x_j を増やすと x_i が減少する
        int leaving_row = -1;
        Rational min_ratio = {-1, 1};
        int min_leaving_idx = -1;

        for (int i = 1; i <= m; i++) {
            if (T[i][entering_col + 1].num < 0) {
                // 比率 = -定数項 / 係数
                Rational ratio = r_div(r_neg(T[i][0]), T[i][entering_col + 1]);
                if (leaving_row == -1 || r_sub(ratio, min_ratio).num < 0) {
                    min_ratio = ratio;
                    min_leaving_idx = row_vars[i];
                    leaving_row = i;
                } else if (r_sub(ratio, min_ratio).num == 0) {
                    if (row_vars[i] < min_leaving_idx) {
                        min_leaving_idx = row_vars[i];
                        leaving_row = i;
                    }
                }
            }
        }

        // 出る変数の制限行が全く見つからない場合は非有界
        if (leaving_row == -1) {
            printf("この問題は非有界です。\n");
            return 1;
        }

        char ent_name[10], lea_name[10];
        format_var_name(col_vars[entering_col], ent_name);
        format_var_name(row_vars[leaving_row], lea_name);
        printf("ピボット演算 %d (出る変数: %s, 入る変数: %s)\n\n", iter++, lea_name, ent_name);

        // 3. ピボット演算（辞書・タブローの更新）
        Rational pivot = T[leaving_row][entering_col + 1];
        
        for (int j = 0; j <= num_cols; j++) {
            if (j != entering_col + 1) {
                T[leaving_row][j] = r_neg(r_div(T[leaving_row][j], pivot));
            }
        }
        T[leaving_row][entering_col + 1] = r_div(make_rational(1, 1), pivot);

        for (int i = 0; i < num_rows; i++) {
            if (i != leaving_row) {
                Rational factor = T[i][entering_col + 1];
                T[i][entering_col + 1] = make_rational(0, 1);
                for (int j = 0; j <= num_cols; j++) {
                    T[i][j] = r_add(T[i][j], r_mul(factor, T[leaving_row][j]));
                }
            }
        }

        if (phase == 1) {
            Rational factor_z = row_z_orig[entering_col + 1];
            row_z_orig[entering_col + 1] = make_rational(0, 1);
            for (int j = 0; j <= num_cols; j++) {
                row_z_orig[j] = r_add(row_z_orig[j], r_mul(factor_z, T[leaving_row][j]));
            }
        }

        // 基底と非基底変数のインデックスをスワップ
        int temp = row_vars[leaving_row];
        row_vars[leaving_row] = col_vars[entering_col];
        col_vars[entering_col] = temp;

        print_tableau(T, row_vars, col_vars, num_rows, num_cols, has_xa, row_z_orig, phase);
    }

    // --- 最終結果の出力 ---
    char buf[64];
    printf("【最終結果】\n");
    printf("最適値 min z = ");
    format_rational(T[0][0], buf);
    printf("%s\n", buf);
    printf("各変数の値:\n");
    
    int total_vars = n_orig + m;
    for (int v = 1; v <= total_vars; v++) {
        int is_basic = 0;
        for (int i = 1; i <= m; i++) {
            if (row_vars[i] == v) {
                format_rational(T[i][0], buf);
                printf("  x%d = %s\n", v, buf);
                is_basic = 1;
                break;
            }
        }
        if (!is_basic) {
            printf("  x%d = 0\n", v);
        }
    }

    // メモリの解放
    for (int i = 0; i < num_rows; i++) free(T[i]);
    free(T); free(row_vars); free(col_vars);
    if (row_z_orig) free(row_z_orig);
    for (int i = 0; i < m; i++) free(input_A[i]);
    free(input_A); free(input_c);

    return 0;
}
