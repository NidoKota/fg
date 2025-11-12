// realize時にリソースの使用情報を活用する例

#include "framegraph.hpp"
#include <iostream>

// 例: テクスチャの記述
struct texture_description 
{
    int width;
    int height;
    int format;
};

// 例: 実際のテクスチャクラス
struct texture 
{
    int width;
    int height;
    int format;
    void* data;
    
    texture(int w, int h, int f) : width(w), height(h), format(f), data(nullptr) {}
};

// realize関数の特殊化 - usage_infoを活用
namespace fg 
{
    template <>
    std::unique_ptr<texture> realize<texture_description, texture>(
        const texture_description &description, 
        const resource_usage_info &usage_info)
    {
        std::cout << "=== Texture Realize ===" << std::endl;
        std::cout << "Size: " << description.width << "x" << description.height << std::endl;
        
        // 1. 自分を作ったタスクを把握
        if (usage_info.creator) 
        {
            std::cout << "\nCreated by task: " << usage_info.creator->name() << std::endl;
            
            // 2. そのタスクが作った他のテクスチャ/リソースを把握
            std::cout << "  Creator task creates " << usage_info.creator->creates().size() << " resources:" << std::endl;
            for (const auto* res : usage_info.creator->creates()) 
            {
                std::cout << "    - " << res->name() << " (ID: " << res->id() << ")" << std::endl;
            }
            
            // そのタスクが読み取るリソース
            if (!usage_info.creator->reads().empty()) 
            {
                std::cout << "  Creator task reads " << usage_info.creator->reads().size() << " resources:" << std::endl;
                for (const auto* res : usage_info.creator->reads()) 
                {
                    std::cout << "    - " << res->name() << " (ID: " << res->id() << ")" << std::endl;
                }
            }
            
            // そのタスクが書き込むリソース
            if (!usage_info.creator->writes().empty()) 
            {
                std::cout << "  Creator task writes " << usage_info.creator->writes().size() << " resources:" << std::endl;
                for (const auto* res : usage_info.creator->writes()) 
                {
                    std::cout << "    - " << res->name() << " (ID: " << res->id() << ")" << std::endl;
                }
            }
        }
        
        // 3. 次に実行されるタスクを把握（このリソースを読み取るタスク）
        std::cout << "\nNext tasks (readers): " << usage_info.readers->size() << " task(s)";
        for (const auto* reader : *usage_info.readers) 
        {
            std::cout << " [" << reader->name() << "]";
        }
        std::cout << std::endl;
        
        // 次に実行されるタスクを把握（このリソースを書き込むタスク）
        if (!usage_info.writers->empty()) 
        {
            std::cout << "Next tasks (writers): " << usage_info.writers->size() << " task(s)";
            for (const auto* writer : *usage_info.writers) 
            {
                std::cout << " [" << writer->name() << "]";
            }
            std::cout << std::endl;
        }
        
        // 使用情報に基づいて最適化の判断が可能
        bool is_read_only = usage_info.writers->empty();
        if (is_read_only) 
        {
            std::cout << "\n-> Read-only resource detected, using optimized format" << std::endl;
        }
        
        // 実際のテクスチャを作成
        return std::make_unique<texture>(description.width, description.height, description.format);
    }
}

// 使用例
void demonstrate_usage_info() 
{
    fg::framegraph graph;
    
    // タスク1: テクスチャを作成
    struct create_task_data 
    {
        fg::resource<texture_description, texture>* output;
    };
    
    auto* create_task = graph.add_render_task<create_task_data>(
        "CreateTexture",
        [](create_task_data& data, fg::render_task_builder& builder) 
        {
            data.output = builder.create<fg::resource<texture_description, texture>>(
                "MyTexture", 
                texture_description{1024, 1024, 0}
            );
        },
        [](const create_task_data& data) 
        {
            std::cout << "Executing CreateTexture task" << std::endl;
        }
    );
    
    // タスク2: テクスチャを読み取る
    struct read_task_data 
    {
        fg::resource<texture_description, texture>* input;
    };
    
    auto* read_task = graph.add_render_task<read_task_data>(
        "ReadTexture",
        [&](read_task_data& data, fg::render_task_builder& builder) 
        {
            data.input = builder.read(create_task->data().output);
        },
        [](const read_task_data& data) 
        {
            std::cout << "Executing ReadTexture task" << std::endl;
        }
    );
    
    // タスク3: テクスチャを書き込む
    struct write_task_data 
    {
        fg::resource<texture_description, texture>* texture;
    };
    
    /*auto* write_task = graph.add_render_task<write_task_data>(
        "WriteTexture",
        [&](write_task_data& data, fg::render_task_builder& builder) 
        {
            data.texture = builder.write(create_task->data().output);
        },
        [](const write_task_data& data) 
        {
            std::cout << "Executing WriteTexture task" << std::endl;
        }
    );*/
    
    // コンパイルして実行
    graph.compile();
    
    // Graphviz出力
    std::cout << "\n=== Exporting Graphviz ===" << std::endl;
    graph.export_graphviz("realize_usage_example.gv");
    std::cout << "Exported to: realize_usage_example.gv" << std::endl;
    
    std::cout << "\n=== Executing Framegraph ===" << std::endl;
    graph.execute();
}

int main() 
{
    demonstrate_usage_info();
    return 0;
}
