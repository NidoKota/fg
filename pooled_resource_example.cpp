#include <stack>
#include <memory>
#include <unordered_map>
#include "include/fg/framegraph.hpp"

// プール管理クラス
template<typename ResourceType>
class resource_pool 
{
private:
    std::stack<std::unique_ptr<ResourceType>> available_resources_;
    
public:
    std::unique_ptr<ResourceType> acquire(const auto& description) 
    {
        if (!available_resources_.empty()) 
        {
            auto resource = std::move(available_resources_.top());
            available_resources_.pop();
            std::cout << "プールからリソースを取得: " << resource.get() << std::endl;
            return resource;
        }
        
        // 新しいリソースを作成
        auto new_resource = std::make_unique<ResourceType>();
        std::cout << "新しいリソースを作成: " << new_resource.get() << std::endl;
        return new_resource;
    }
    
    void release(std::unique_ptr<ResourceType> resource) 
    {
        if (resource) 
        {
            std::cout << "リソースをプールに返却: " << resource.get() << std::endl;
            available_resources_.push(std::move(resource));
        }
    }
    
    size_t pool_size() const 
    {
        return available_resources_.size();
    }
};

// グローバルプールインスタンス
static resource_pool<gl::buffer> buffer_pool;
static resource_pool<gl::texture_2d> texture_pool;

// プール対応のカスタムリソースクラス
template<typename description_type_, typename actual_type_>
class pooled_resource : public fg::resource_base 
{
public:
    using description_type = description_type_;
    using actual_type = actual_type_;

    explicit pooled_resource(const std::string& name, const fg::render_task_base* creator, const description_type& description) 
        : fg::resource_base(name, creator), description_(description) {}
    
    explicit pooled_resource(const std::string& name, const description_type& description, actual_type* actual = nullptr) 
        : fg::resource_base(name, nullptr), description_(description), actual_ptr_(actual) {}

    const description_type& description() const { return description_; }
    
    actual_type* actual() const 
    { 
        return actual_ptr_.get(); 
    }

protected:
    void realize() override 
    {
        if (transient()) 
        {
            // プールから取得（特殊化された関数を呼び出し）
            if constexpr (std::is_same_v<actual_type, gl::buffer>) 
            {
                actual_ptr_ = buffer_pool.acquire(description_);
            } else if constexpr (std::is_same_v<actual_type, gl::texture_2d>) 
            {
                actual_ptr_ = texture_pool.acquire(description_);
            }
        }
    }
    
    void derealize() override 
    {
        if (transient() && actual_ptr_) 
        {
            // プールに返却
            if constexpr (std::is_same_v<actual_type, gl::buffer>) 
            {
                buffer_pool.release(std::move(actual_ptr_));
            } else if constexpr (std::is_same_v<actual_type, gl::texture_2d>) 
            {
                texture_pool.release(std::move(actual_ptr_));
            }
        }
    }

private:
    description_type description_;
    std::unique_ptr<actual_type> actual_ptr_;
};

// プール対応リソース型の定義
using pooled_buffer_resource = pooled_resource<glr::buffer_description, gl::buffer>;
using pooled_texture_2d_resource = pooled_resource<glr::texture_description, gl::texture_2d>;

// 使用例
void demonstrate_pooling() {
    std::cout << "=== プール対応フレームグラフのデモ ===" << std::endl;
    
    fg::framegraph framegraph;
    
    // プール統計を表示する関数
    auto print_pool_stats = []() {
        std::cout << "プール統計 - バッファ: " << buffer_pool.pool_size() 
                  << ", テクスチャ: " << texture_pool.pool_size() << std::endl;
    };
    
    print_pool_stats();
    
    // 複数回実行してプールの効果を確認
    for (int frame = 0; frame < 3; ++frame) 
    {
        std::cout << "\n--- フレーム " << frame + 1 << " ---" << std::endl;
        
        framegraph.clear(); // 前のフレームをクリア
        
        // レンダータスクを追加（プール対応リソースを使用）
        struct task_data 
        {
            pooled_buffer_resource* buffer;
            pooled_texture_2d_resource* texture;
        };
        
        auto task = framegraph.add_render_task<task_data>(
            "Pooled Task",
            [&](task_data& data, fg::render_task_builder& builder) 
            {
                // カスタムリソース作成関数が必要
                // 実際の実装では render_task_builder の拡張が必要
            },
            [=](const task_data& data) 
            {
                std::cout << "タスク実行中..." << std::endl;
            }
        );
        
        framegraph.compile();
        framegraph.execute();
        
        print_pool_stats();
    }
}
