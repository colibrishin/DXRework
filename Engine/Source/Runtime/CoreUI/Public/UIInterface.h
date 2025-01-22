#pragma once
#include <filesystem>
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
            std::ranges::for_each(contexts, [&](UITokenBase* new_child)
            {
	            new_child->m_parent_ = this;
            });
            return *this;
        }

        // Add child and returns added child.
        UITokenBase& AddChild(UITokenBase* child)
        {
            m_children_.push_back(std::unique_ptr<UITokenBase>(child));
            child->m_parent_ = this;
        	return *child;
        }

        UITokenBase& operator+=(UITokenBase* child)
        {
	        return AddChild(child);
        }

        UITokenBase* GetParentInternal() const
        {
	        return m_parent_;
        }

        virtual void Do() const = 0;
        virtual void End() const = 0;

    protected:
        std::function<void()>                     m_function_;
        UITokenBase*                              m_parent_ = nullptr;
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

    struct ENGINE_COREUI_API DialogToken : UIToken<const void*, const std::string_view, bool&>
    {
        explicit DialogToken(const void* context, const std::string_view title, bool& opened)
            : UIToken(context, title, opened) {}
    };

    struct ENGINE_COREUI_API ButtonToken : UIToken<const std::string_view>
    {
        explicit ButtonToken(const std::string_view title)
            : UIToken(title) {}
    };

    struct ENGINE_COREUI_API LabelAndTextToken : UIToken<const std::string_view, std::string&, bool>
    {
        explicit LabelAndTextToken(const std::string_view title, std::string& target, bool editable)
            : UIToken(title, target, editable) {}
    };

    struct ENGINE_COREUI_API LabelAndPathToken : UIToken<const std::string_view, const std::filesystem::path&>
    {
        explicit LabelAndPathToken(const std::string_view title, const std::filesystem::path& path)
            : UIToken(title, path) {}
    };

    struct ENGINE_COREUI_API LabelAndFloatToken : UIToken<const std::string_view, float&, float, float, bool>
    {
        explicit LabelAndFloatToken(const std::string_view title, float& target, float step, float speed, bool editable)
            : UIToken(title, target, step, speed, editable) {}
    };

    struct ENGINE_COREUI_API LabelAndIntToken : UIToken<const std::string_view, int&, bool>
    {
        explicit LabelAndIntToken(const std::string_view title, int& target, bool editable)
            : UIToken(title, target, editable) {}
    };

    struct ENGINE_COREUI_API LabelAndUIntToken : UIToken<const std::string_view, uint32_t&, bool>
    {
        explicit LabelAndUIntToken(const std::string_view title, uint32_t& target, bool editable)
            : UIToken(title, target, editable) {}
    };

    struct ENGINE_COREUI_API LabelAndULLDToken : UIToken<const std::string_view, uint64_t&, bool>
    {
        explicit LabelAndULLDToken(const std::string_view title, uint64_t& target, bool editable)
            : UIToken(title, target, editable) {}
    };

    struct ENGINE_COREUI_API ListBoxToken : UIToken<const std::string_view, float, float>
    {
	    ListBoxToken(const std::string_view label, float x, float y)
		    : UIToken<const std::string_view, float, float>(label, x, y) {}
    };

    struct ENGINE_COREUI_API TreeNodeToken : UIToken<const std::string_view>
    {
	    explicit TreeNodeToken(const std::string_view label)
		    : UIToken<const std::string_view>(label) {}
    };

    struct ENGINE_COREUI_API SelectableToken : UIToken<const std::string_view, bool&>
    {
	    SelectableToken(const std::string_view basic_string_view, bool& cond)
		    : UIToken<const std::string_view, bool&>(basic_string_view, cond) {}
    };

    struct UIContext
    {
        explicit UIContext(UITokenBase* parent)
        {
	        m_parent_ = std::unique_ptr<UITokenBase>(parent);
            m_active_child_ = nullptr;
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

        // Add Child and return this
        UITokenBase& operator<<(UITokenBase* child)
        {
            m_parent_->AddChild(child);
            m_active_child_ = child;
	        return *m_parent_;
        }

        UITokenBase& operator<<(const std::function<void()>& functor) const
        {
            m_parent_->SetFunction(functor);
	        return *m_parent_;
        }

        // Add Child and return child
        UITokenBase& operator+=(UITokenBase* child)
        {
            if (m_active_child_)
            {
                UITokenBase* old_active = m_active_child_;
				old_active->AddChild(child);
                m_active_child_ = child;
                return *old_active;
            }
            else
            {
                operator<<(child);
                return *m_parent_;
            }
        }

        // Add Child to active child without swapping active child.
        UITokenBase& operator|=(UITokenBase* child) const
        {
	        if (m_active_child_)
	        {
		        m_active_child_->AddChild(child);
                return *child;
	        }
            else
            {
	            m_parent_->AddChild(child);
                return *child;
            }
        }

        UITokenBase& operator+=(const std::function<void()>& functor) const
        {
            if (m_active_child_)
            {
				m_active_child_->SetFunction(functor);
                return *m_active_child_;
            }
            else
            {
	            m_parent_->SetFunction(functor);
                return *m_parent_;
            }
        }

        UITokenBase& operator--()
        {
	        m_active_child_ = m_active_child_->GetParentInternal();
            return *m_active_child_;
        }

    private:
        std::unique_ptr<UITokenBase> m_parent_;
        UITokenBase* m_active_child_ = nullptr;
    };

    struct ENGINE_COREUI_API UIInterface
    {
        virtual ~UIInterface() = default;

        [[nodiscard]] static UIContext NewContext(UITokenBase* root)
        {
            return UIContext(root);
        }

        virtual UITokenBase*  NewMainMenuBar(const MainMenuBarToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase*         NewMenu(const MenuToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase*     NewMenuItem(const MenuItemToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase*       NewDialog(const DialogToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase*       NewButton(const ButtonToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewLabelAndText(const LabelAndTextToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewLabelAndFloat(const LabelAndFloatToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewLabelAndInt(const LabelAndIntToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewLabelAndUInt(const LabelAndUIntToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewLabelAndULLD(const LabelAndULLDToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewLabelAndPath(const LabelAndPathToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewListBox(const ListBoxToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewTreeNode(const TreeNodeToken::ArgumentTuple& arguments) = 0;
        virtual UITokenBase* NewSelectable(const SelectableToken::ArgumentTuple& arguments) = 0;

        virtual void NewFrame() = 0;

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

        static void Shutdown()
        {
	        if (IsValid())
	        {
		        m_ui_interface_.reset();
	        }
        }
        
    private:
        static std::unique_ptr<UIInterface> m_ui_interface_;
    };
}
