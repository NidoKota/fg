#include <iostream>
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

// リソース取得の特殊化のためのトレイト
template<typename ResourceType>
struct resource_pool_traits 
{
    static resource_pool<ResourceType>& get_pool();
};

// 各リソース型に対するプールの特殊化
template<>
struct resource_pool_traits<gl::buffer> 
{
    static resource_pool<gl::buffer>& get_pool() 
    {
        static resource_pool<gl::buffer> pool;
        return pool;
    }
};

template<>
struct resource_pool_traits<gl::texture_2d> 
{
    static resource_pool<gl::texture_2d>& get_pool() 
    {
        static resource_pool<gl::texture_2d> pool;
        return pool;
    }
};

// 新しいリソース型の例（簡単に追加可能）
template<>
struct resource_pool_traits<gl::render_buffer> 
{
    static resource_pool<gl::render_buffer>& get_pool() 
    {
        static resource_pool<gl::render_buffer> pool;
        return pool;
    }
};

// プール対応のカスタムリソースクラス（特殊化版）
template<typename description_type_, typename actual_type_>
class specialized_pooled_resource : public fg::resource_base 
{
public:
    using description_type = description_type_;
    using actual_type = actual_type_;

    explicit specialized_pooled_resource(const std::string& name, const fg::render_task_base* creator, const description_type& description) 
        : fg::resource_base(name, creator), description_(description) {}
    
    explicit specialized_pooled_resource(const std::string& name, const description_type& description, actual_type* actual = nullptr) 
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
            // 特殊化されたプールから取得
            auto& pool = resource_pool_traits<actual_type>::get_pool();
            actual_ptr_ = pool.acquire(description_);
        }
    }
    
    void derealize() override 
    {
        if (transient() && actual_ptr_) 
        {
            // 特殊化されたプールに返却
            auto& pool = resource_pool_traits<actual_type>::get_pool();
            pool.release(std::move(actual_ptr_));
        }
    }

private:
    description_type description_;
    std::unique_ptr<actual_type> actual_ptr_;
};

// さらに高度な特殊化：リソース作成ロジックも特殊化可能
template<typename ResourceType>
struct resource_factory 
{
    template<typename DescriptionType>
    static std::unique_ptr<ResourceType> create(const DescriptionType& description) 
    {
        return std::make_unique<ResourceType>();
    }
};

// バッファ用の特殊化された作成ロジック
template<>
struct resource_factory<gl::buffer> 
{
    template<typename DescriptionType>
    static std::unique_ptr<gl::buffer> create(const DescriptionType& description) 
    {
        auto buffer = std::make_unique<gl::buffer>();
        // バッファ固有の初期化ロジック
        std::cout << "バッファを特殊化された方法で作成" << std::endl;
        return buffer;
    }
};

// テクスチャ用の特殊化された作成ロジック
template<>
struct resource_factory<gl::texture_2d> 
{
    template<typename DescriptionType>
    static std::unique_ptr<gl::texture_2d> create(const DescriptionType& description) 
    {
        auto texture = std::make_unique<gl::texture_2d>();
        // テクスチャ固有の初期化ロジック
        std::cout << "テクスチャを特殊化された方法で作成" << std::endl;
        return texture;
    }
};

// ファクトリーを使用する高度なプール
template<typename ResourceType>
class advanced_resource_pool 
{
private:
    std::stack<std::unique_ptr<ResourceType>> available_resources_;
    
public:
    template<typename DescriptionType>
    std::unique_ptr<ResourceType> acquire(const DescriptionType& description) 
    {
        if (!available_resources_.empty()) 
        {
            auto resource = std::move(available_resources_.top());
            available_resources_.pop();
            std::cout << "プールからリソースを取得: " << resource.get() << std::endl;
            return resource;
        }
        
        // 特殊化されたファクトリーで作成
        return resource_factory<ResourceType>::create(description);
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

// 高度なプール用のトレイト
template<typename ResourceType>
struct advanced_pool_traits 
{
    static advanced_resource_pool<ResourceType>& get_pool();
};

template<>
struct advanced_pool_traits<gl::buffer> 
{
    static advanced_resource_pool<gl::buffer>& get_pool() 
    {
        static advanced_resource_pool<gl::buffer> pool;
        return pool;
    }
};

template<>
struct advanced_pool_traits<gl::texture_2d> 
{
    static advanced_resource_pool<gl::texture_2d>& get_pool() 
    {
        static advanced_resource_pool<gl::texture_2d> pool;
        return pool;
    }
};

// 高度な特殊化リソースクラス
template<typename description_type_, typename actual_type_>
class advanced_pooled_resource : public fg::resource_base 
{
public:
    using description_type = description_type_;
    using actual_type = actual_type_;

    explicit advanced_pooled_resource(const std::string& name, const fg::render_task_base* creator, const description_type& description) 
        : fg::resource_base(name, creator), description_(description) {}
    
    explicit advanced_pooled_resource(const std::string& name, const description_type& description, actual_type* actual = nullptr) 
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
            // 特殊化されたプールとファクトリーから取得
            auto& pool = advanced_pool_traits<actual_type>::get_pool();
            actual_ptr_ = pool.acquire(description_);
        }
    }
    
    void derealize() override 
    {
        if (transient() && actual_ptr_) 
        {
            // 特殊化されたプールに返却
            auto& pool = advanced_pool_traits<actual_type>::get_pool();
            pool.release(std::move(actual_ptr_));
        }
    }

private:
    description_type description_;
    std::unique_ptr<actual_type> actual_ptr_;
};

// 便利なエイリアス
using specialized_buffer_resource = specialized_pooled_resource<glr::buffer_description, gl::buffer>;
using specialized_texture_2d_resource = specialized_pooled_resource<glr::texture_description, gl::texture_2d>;
using specialized_render_buffer_resource = specialized_pooled_resource<glr::render_buffer_description, gl::render_buffer>;

using advanced_buffer_resource = advanced_pooled_resource<glr::buffer_description, gl::buffer>;
using advanced_texture_2d_resource = advanced_pooled_resource<glr::texture_description, gl::texture_2d>;

// 使用例とデモ
void demonstrate_specialized_pooling() 
{
    std::cout << "=== 特殊化プール対応フレームグラフのデモ ===" << std::endl;
    
    // プール統計を表示する関数
    auto print_pool_stats = []() 
    {
        auto& buffer_pool = resource_pool_traits<gl::buffer>::get_pool();
        auto& texture_pool = resource_pool_traits<gl::texture_2d>::get_pool();
        auto& render_buffer_pool = resource_pool_traits<gl::render_buffer>::get_pool();
        
        std::cout << "プール統計 - バッファ: " << buffer_pool.pool_size() 
                  << ", テクスチャ: " << texture_pool.pool_size()
                  << ", レンダーバッファ: " << render_buffer_pool.pool_size() << std::endl;
    };
    
    print_pool_stats();
    
    // 新しいリソース型を簡単にテスト
    {
        std::cout << "\n--- 新しいリソース型のテスト ---" << std::endl;
        
        glr::render_buffer_description rb_desc;
        specialized_render_buffer_resource rb_resource("test_rb", rb_desc);
        
        // リソースの実現とデリアライズをシミュレート
        // 実際の使用では framegraph が管理
        std::cout << "レンダーバッファリソースが正常に作成されました" << std::endl;
    }
    
    print_pool_stats();
}

void demonstrate_advanced_pooling() 
{
    std::cout << "\n=== 高度な特殊化プール対応フレームグラフのデモ ===" << std::endl;
    
    // 高度なプール統計を表示する関数
    auto print_advanced_pool_stats = []() 
    {
        auto& buffer_pool = advanced_pool_traits<gl::buffer>::get_pool();
        auto& texture_pool = advanced_pool_traits<gl::texture_2d>::get_pool();
        
        std::cout << "高度なプール統計 - バッファ: " << buffer_pool.pool_size() 
                  << ", テクスチャ: " << texture_pool.pool_size() << std::endl;
    };
    
    print_advanced_pool_stats();
    
    // 特殊化されたファクトリーのテスト
    {
        std::cout << "\n--- 特殊化ファクトリーのテスト ---" << std::endl;
        
        glr::buffer_description buf_desc;
        glr::texture_description tex_desc;
        
        advanced_buffer_resource buffer_resource("test_buffer", buf_desc);
        advanced_texture_2d_resource texture_resource("test_texture", tex_desc);
        
        std::cout << "高度なリソースが正常に作成されました" << std::endl;
    }
    
    print_advanced_pool_stats();
}

int main() 
{
    demonstrate_specialized_pooling();
    demonstrate_advanced_pooling();
    return 0;
}
