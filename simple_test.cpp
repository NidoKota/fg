// 標準ライブラリのインクルード
#include <array>      // std::arrayのため
#include <cstddef>    // std::size_tのため
#include <iostream>   // 標準入出力のため

// フレームグラフライブラリのインクルード
#include "include/fg/framegraph.hpp"

// OpenGL リソースハンドルの型定義
// 実際のOpenGLオブジェクトのIDを表現するための型エイリアス
namespace gl
{
    using buffer = std::size_t;        // バッファオブジェクトのハンドル
    using texture_1d = std::size_t;     // 1Dテクスチャオブジェクトのハンドル
    using texture_2d = std::size_t;     // 2Dテクスチャオブジェクトのハンドル
    using texture_3d = std::size_t;     // 3Dテクスチャオブジェクトのハンドル
}

// OpenGL リソース記述子とフレームグラフリソース型の定義
// リソースの作成に必要な情報とフレームグラフで使用するリソース型を定義
namespace glr
{
    struct buffer_description
    {
        std::size_t size;               // バッファのサイズ（バイト単位）
    };
    struct texture_description
    {
        std::size_t levels;             // ミップマップレベル数
        std::size_t format;             // テクスチャフォーマット（例：GL_RGBA8）
        std::array<std::size_t, 3> size; // テクスチャサイズ [幅, 高さ, 深度]
    };

    // フレームグラフで使用するリソース型の定義
    using buffer_resource = fg::resource<buffer_description, gl::buffer>;           // バッファリソース
    using texture_1d_resource = fg::resource<texture_description, gl::texture_1d>;  // 1Dテクスチャリソース
    using texture_2d_resource = fg::resource<texture_description, gl::texture_2d>;  // 2Dテクスチャリソース
    using texture_3d_resource = fg::resource<texture_description, gl::texture_3d>;  // 3Dテクスチャリソース
}

// フレームグラフのリソース実現化関数の特殊化
// リソース記述子から実際のOpenGLオブジェクトを生成する処理を定義
namespace fg
{
    // バッファリソースの実現化関数
    template<>
    std::unique_ptr<gl::buffer> realize(const glr::buffer_description& description)
    {
        return std::make_unique<gl::buffer>(description.size);  // 指定サイズでバッファを作成
    }
    // 2Dテクスチャリソースの実現化関数
    template<>
    std::unique_ptr<gl::texture_2d> realize(const glr::texture_description& description)
    {
        return std::make_unique<gl::buffer>(description.levels); // レベル数でテクスチャを作成
    }
}

int main()
{
    std::cout << "Framegraph テストを開始します..." << std::endl;

    // フレームグラフインスタンスの作成
    fg::framegraph framegraph;

    // 永続リソースの追加（フレーム間で保持されるリソース）
    auto retained_resource = framegraph.add_retained_resource("Retained Resource 1", glr::texture_description(), static_cast<gl::texture_2d*>(nullptr));

    // 最初のレンダータスクの宣言
    struct render_task_1_data
    {
        glr::texture_2d_resource* output1;  // 出力テクスチャ1
        glr::texture_2d_resource* output2;  // 出力テクスチャ2
        glr::texture_2d_resource* output3;  // 出力テクスチャ3
        glr::texture_2d_resource* output4;  // 出力テクスチャ4（永続リソース）
    };
    // レンダータスク1の追加（セットアップと実行関数を指定）
    auto render_task_1 = framegraph.add_render_task<render_task_1_data>(
        "Render Task 1",
        // セットアップ関数：リソースの作成と依存関係を定義
        [&] (render_task_1_data& data, fg::render_task_builder& builder)
        {
            data.output1 = builder.create<glr::texture_2d_resource>("Resource 1", glr::texture_description()); // 新しいリソース1を作成
            data.output2 = builder.create<glr::texture_2d_resource>("Resource 2", glr::texture_description()); // 新しいリソース2を作成
            data.output3 = builder.create<glr::texture_2d_resource>("Resource 3", glr::texture_description()); // 新しいリソース3を作成
            data.output4 = builder.write<glr::texture_2d_resource>(retained_resource);                        // 永続リソースへの書き込み
        },
        // 実行関数：実際のレンダリング処理を実行
        [=] (const render_task_1_data& data)
        {
            std::cout << "Render Task 1 を実行中..." << std::endl;
            // 実際のレンダリング処理。CPUからリソースをロードできる
            auto actual1 = data.output1->actual();  // 実際のOpenGLオブジェクトを取得
            auto actual2 = data.output2->actual();  // 実際のOpenGLオブジェクトを取得
            auto actual3 = data.output3->actual();  // 実際のOpenGLオブジェクトを取得
            auto actual4 = data.output4->actual();  // 実際のOpenGLオブジェクトを取得
        });

    // レンダータスク1のデータへの参照を取得（他のタスクから参照するため）
    auto& data_1 = render_task_1->data();

    // 2番目のレンダータスクの宣言
    struct render_task_2_data
    {
        glr::texture_2d_resource* input1;   // 入力テクスチャ1（タスク1の出力を使用）
        glr::texture_2d_resource* input2;   // 入力テクスチャ2（タスク1の出力を使用）
        glr::texture_2d_resource* output1;  // 出力テクスチャ1（タスク1のリソースを再利用）
        glr::texture_2d_resource* output2;  // 出力テクスチャ2（新規作成）
    };
    // レンダータスク2の追加
    auto render_task_2 = framegraph.add_render_task<render_task_2_data>(
        "Render Task 2",
        // セットアップ関数：タスク1の出力を入力として使用
        [&] (render_task_2_data& data, fg::render_task_builder& builder)
        {
            data.input1 = builder.read(data_1.output1);                                                     // タスク1の出力1を読み取り
            data.input2 = builder.read(data_1.output2);                                                     // タスク1の出力2を読み取り
            data.output1 = builder.write(data_1.output3);                                                   // タスク1の出力3を書き込み用として使用
            data.output2 = builder.create<glr::texture_2d_resource>("Resource 4", glr::texture_description()); // 新しいリソース4を作成
        },
        // 実行関数：タスク1の結果を使用して処理
        [=] (const render_task_2_data& data)
        {
            std::cout << "Render Task 2 を実行中..." << std::endl;
            // 実際のレンダリング処理。CPUからリソースをロードできる
            auto actual1 = data.input1->actual();   // 入力リソース1の実際のオブジェクト
            auto actual2 = data.input2->actual();   // 入力リソース2の実際のオブジェクト
            auto actual3 = data.output1->actual();  // 出力リソース1の実際のオブジェクト
            auto actual4 = data.output2->actual();  // 出力リソース2の実際のオブジェクト
        });

    // レンダータスク2のデータへの参照を取得
    auto& data_2 = render_task_2->data();

    // 3番目のレンダータスクの宣言（最終出力タスク）
    struct render_task_3_data
    {
        glr::texture_2d_resource* input1;  // 入力テクスチャ1（タスク2の出力を使用）
        glr::texture_2d_resource* input2;  // 入力テクスチャ2（タスク2の出力を使用）
        glr::texture_2d_resource* output;  // 最終出力（永続リソースへ書き込み）
    };
    // 最終レンダータスクの追加
    auto render_task_3 = framegraph.add_render_task<render_task_3_data>(
        "Render Task 3",
        // セットアップ関数：タスク2の結果を永続リソースに出力
        [&] (render_task_3_data& data, fg::render_task_builder& builder)
        {
            data.input1 = builder.read(data_2.output1);    // タスク2の出力1を読み取り
            data.input2 = builder.read(data_2.output2);    // タスク2の出力2を読み取り
            data.output = builder.write(retained_resource); // 永続リソースへの最終出力
        },
        // 実行関数：最終結果を永続リソースに出力
        [=] (const render_task_3_data& data)
        {
            std::cout << "Render Task 3 を実行中..." << std::endl;
            // 実際のレンダリング処理。CPUからリソースをロードできる
            auto actual1 = data.input1->actual();  // 入力リソース1の実際のオブジェクト
            auto actual2 = data.input2->actual();  // 入力リソース2の実際のオブジェクト
            auto actual3 = data.output->actual();  // 最終出力リソースの実際のオブジェクト
        });

    // フレームグラフのコンパイル（タスクの依存関係を解析し、実行順序を決定）
    std::cout << "フレームグラフをコンパイル中..." << std::endl;
    framegraph.compile();

    // フレームグラフの実行（3回繰り返してテスト）
    std::cout << "フレームグラフを実行中..." << std::endl;
    for(auto i = 0; i < 3; i++)  // 3フレーム分実行
        framegraph.execute();

    // GraphViz形式でフレームグラフの構造をエクスポート
    std::cout << "GraphVizファイルを出力中..." << std::endl;
    framegraph.export_graphviz("framegraph.gv");

    std::cout << "テスト完了！framegraph.gv ファイルが生成されました。" << std::endl;

    // フレームグラフのクリーンアップ
    framegraph.clear();
    return 0;
}
