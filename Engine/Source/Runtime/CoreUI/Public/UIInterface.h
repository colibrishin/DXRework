#pragma once
#include <functional>
#include <memory>
#include <string_view>
#include <vector>
#include <tuple>

#include "../Misc.h"

namespace Engine
{
	struct ENGINE_COREUI_API UITokenBase
    {
        virtual ~UITokenBase() = default;

        UITokenBase() = default;
        UITokenBase(UITokenBase&) = delete;
        UITokenBase& operator=(UITokenBase&) = delete;

        UITokenBase& SetFunction(const std::function<void()>& function)
        {
            m_function_ = function;
            return *this;
        }

        // Add children and return this.
        UITokenBase& AddChildren(const std::initializer_list<UITokenBase*>& contexts)
        {
            m_children_.insert(m_children_.end(), contexts.begin(), contexts.end());
            return *this;
        }

        // Add child and returns added child.
        UITokenBase& AddChild(UITokenBase* child)
        {
            m_children_.push_back(std::unique_ptr<UITokenBase>(child));
        	return *child;
        }

        virtual void Do() const = 0;
        virtual void End() const = 0;

    protected:
        std::function<void()>                           m_function_;
        std::vector<std::unique_ptr<UITokenBase>> m_children_;
    };
    
    template <typename... Args>
    struct UIToken : public UITokenBase
    {
        using ArgumentTuple = std::tuple<Args...>;
        using ArgumentCount = std::integral_constant<size_t, sizeof...(Args)>;

        ~UIToken() override {}
        
        explicit UIToken(Args... args) : m_tuple_(std::forward_as_tuple(args...)) {}


        void Do() const override
        {
            if (ForwardTuple())
            {
                if (m_function_)
                {
					m_function_();
                }

                for (const std::unique_ptr<UITokenBase>& context : m_children_)
	            {
	                context->Do();
	            }

                End();
            }
        }

    protected:
        [[nodiscard]] virtual bool DoImpl(Args... args) const = 0;

    private:
        [[nodiscard]] bool ForwardTuple() const
        {
            if constexpr (ArgumentCount::value == 0)
            {
	            return DoImpl();
            }

            return ForwardTupleImpl(m_tuple_, std::make_index_sequence<ArgumentCount::value>{});
        }
        
        template <size_t... Is>
        [[nodiscard]] bool ForwardTupleImpl(const ArgumentTuple& tuple, std::index_sequence<Is...>) const
        {
            return DoImpl(std::get<Is>(tuple)...);
        }
        
        ArgumentTuple m_tuple_;
    };

    struct ENGINE_COREUI_API MainMenuBarToken : UIToken<>
    {
        explicit MainMenuBarToken() = default;
    };

    struct ENGINE_COREUI_API MenuToken : UIToken<const std::string_view>
    {
        explicit MenuToken(const std::string_view title)
            : UIToken(title) {}
    };

    struct ENGINE_COREUI_API MenuItemToken : UIToken<const std::string_view>
    {
        explicit MenuItemToken(const std::string_view title)
            : UIToken(title) {}
    };

    struct ENGINE_COREUI_API DialogToken : UIToken<const std::string_view, bool&>
    {
        explicit DialogToken(const std::string_view title, bool& opened)
            : UIToken(title, opened) {}
    };

    struct ENGINE_COREUI_API ButtonToken : UIToken<const std::string_view>
    {
        explicit ButtonToken(const std::string_view title)
            : UIToken(title) {}
    };

    struct ENGINE_COREUI_API TextEditableToken : UIToken<const std::string_view, std::string&>
    {
        explicit TextEditableToken(const std::string_view title, std::string& target)
            : UIToken(title, target) {}
    };

    struct UIContext
    {
        explicit UIContext(UITokenBase* parent)
        {
	        m_parent_ = std::unique_ptr<UITokenBase>(parent);
        }

        UIContext(UIContext&) = delete;
        UIContext& operator=(UIContext&) = delete;

        ~UIContext()
        {
        	m_parent_->Do();
        }

        explicit operator bool() const
        {
	        return m_parent_ != nullptr;
        }

        // Returns this
        UITokenBase& operator()() const
        {
	        return *m_parent_;
        }

        // Add multiple child to this
        template <typename... Args>
        UITokenBase& operator[](Args&&... args)
        {
	        m_parent_->AddChildren(args...);
            return *m_parent_;
        }

    private:
        std::unique_ptr<UITokenBase> m_parent_;
    };

    struct ENGINE_COREUI_API UIInterface
    {
        virtual ~UIInterface() = default;

        [[nodiscard]] static UIContext NewContext(UITokenBase* root)
        {
            return UIContext(root);
        }

        virtual MainMenuBarToken* NewMainMenuBar(const MainMenuBarToken::ArgumentTuple& arguments) = 0;
        virtual MenuToken*        NewMenu(const MenuToken::ArgumentTuple& arguments) = 0;
        virtual MenuItemToken*    NewMenuItem(const MenuItemToken::ArgumentTuple& arguments) = 0;
        virtual DialogToken*      NewDialog(const DialogToken::ArgumentTuple& arguments) = 0;

        virtual void NewFrame() = 0;
        //virtual UITokenBase* NewButton(const std::string_view title) = 0;
        //virtual UITokenBase* NewTextEditable(const std::string_view title) = 0;

    protected:
        template <typename T>
        T* Generate(const typename T::ArgumentTuple& args)
        {
            return NewForwardTuple<T>(args, std::make_index_sequence<T::ArgumentCount::value>{});
        }

        template <typename T, typename Tuple, size_t... Is>
        T* NewForwardTuple(const Tuple& t, std::index_sequence<Is...>)
        {
            return new T(std::get<Is>(t)...);
        }

        // todo: memory pool;
    };

    struct ENGINE_COREUI_API UIInterfaceAccessor final
    {
        template <typename T> requires (std::is_base_of_v<UIInterface, T>)
        static void SetInterface()
        {
            if (!m_ui_interface_)
            {
                m_ui_interface_ = std::make_unique<T>();
            }
        }

        static void NewFrame()
        {
            if (m_ui_interface_)
            {
                m_ui_interface_->NewFrame();
            }
        }

        static bool IsValid()
        {
            return m_ui_interface_ != nullptr;
        }

        [[nodiscard]] static UIInterface& GetInterface()
        {
            return *m_ui_interface_;
        }
        
    private:
        static std::unique_ptr<UIInterface> m_ui_interface_;
    };
}
