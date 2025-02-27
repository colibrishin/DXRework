#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>
#include <tuple>
#include <numeric>
#include <set>

#include "CoreType.h"

namespace Engine
{
    struct ENGINE_CORE_API UIInfo
    {
        std::string label{};
        bool        dialogOpened = false;
        std::unordered_map<std::string, std::string> temporaryStrings{};
    };

	struct ENGINE_CORE_API IUITokenBase
    {
        virtual ~IUITokenBase() = default;

        IUITokenBase(const void* context, const std::string_view name) : m_context_(context), m_name_( name ) {}
        
        IUITokenBase(IUITokenBase&) = delete;
        IUITokenBase& operator=(IUITokenBase&) = delete;

        IUITokenBase& SetFunction(const std::function<void()>& function)
        {
            m_function_ = function;
            return *this;
        }

        // Add children and return this.
        IUITokenBase& AddChildren(const std::initializer_list<IUITokenBase*>& contexts)
        {
            m_children_.insert(m_children_.end(), contexts.begin(), contexts.end());
            std::ranges::for_each(contexts, [&](IUITokenBase* new_child)
            {
	            new_child->m_parent_ = this;
            });
            return *this;
        }

        // Add child and returns added child.
        IUITokenBase& AddChild(IUITokenBase* child)
        {
            m_children_.push_back(std::unique_ptr<IUITokenBase>(child));
            child->m_parent_ = this;
        	return *child;
        }

        IUITokenBase& operator+=(IUITokenBase* child)
        {
	        return AddChild(child);
        }

        IUITokenBase* GetParentInternal() const
        {
	        return m_parent_;
        }

        [[nodiscard]] std::string GetIdentifier() const
        {
            return std::to_string(reinterpret_cast<uint64_t>(m_context_)) + m_name_;
        }

        virtual void Do() = 0;
        virtual void End() = 0;

    protected:
        const void*                               m_context_ = nullptr;
        std::string                               m_name_;
        std::function<void()>                     m_function_;
        IUITokenBase*                              m_parent_ = nullptr;
        std::vector<std::unique_ptr<IUITokenBase>> m_children_;
    };
    
    template <typename... Args>
    struct IUIToken : public IUITokenBase
    {
        using ArgumentTuple = std::tuple<Args...>;
        using ArgumentCount = std::integral_constant<size_t, sizeof...( Args )>;

        ~IUIToken() override
        {}

        IUIToken( const void *context, const std::string_view name, Args... args )
            : IUITokenBase( context, name ),
              m_tuple_( std::forward_as_tuple( args... ) )
        {}

        void Do() override
        {
            if ( ForwardTuple() )
            {
                if ( m_function_ )
                {
                    m_function_();
                }

                for ( const std::unique_ptr<IUITokenBase> &context : m_children_ )
                {
                    context->Do();
                }

                End();
            }
        }

    protected:
        [[nodiscard]] virtual bool DoImpl( Args... args ) = 0;

    private:
        [[nodiscard]] bool ForwardTuple()
        {
            if constexpr ( ArgumentCount::value == 0 )
            {
                return DoImpl();
            }

            return ForwardTupleImpl( m_tuple_, std::make_index_sequence<ArgumentCount::value>{} );
        }

        template <size_t... Is>
        [[nodiscard]] bool ForwardTupleImpl( const ArgumentTuple &tuple, std::index_sequence<Is...> )
        {
            return DoImpl( std::get<Is>( tuple )... );
        }

        ArgumentTuple m_tuple_;
    };

    using UITokenInputContext = uint64_t;

#define NEW_TOKEN_DECL(Name, ...) \
    struct ENGINE_CORE_API Name##Token : IUIToken<__VA_ARGS__> \
    {   \
        using IUIToken<__VA_ARGS__>::IUIToken; \
    };

    NEW_TOKEN_DECL( MainMenuBar )
    NEW_TOKEN_DECL( Menu, const std::string_view )
    NEW_TOKEN_DECL( MenuItem, const std::string_view )
    NEW_TOKEN_DECL( Dialog, const std::string_view, bool & )
    NEW_TOKEN_DECL( Button, const std::string_view )
    NEW_TOKEN_DECL( LabelAndText, const std::string_view, std::string&, bool )
    NEW_TOKEN_DECL( LabelAndPath, const std::string_view, const std::filesystem::path& )
    NEW_TOKEN_DECL( ListBox, const std::string_view, float, float )
    NEW_TOKEN_DECL( TreeNode, const std::string_view )
    NEW_TOKEN_DECL( Selectable, const std::string_view, bool& )
    NEW_TOKEN_DECL( Checkbox, const std::string_view, bool&, const bool )
    NEW_TOKEN_DECL( Combobox, const std::string_view, int*, const char* const*, const size_t, const bool )
    NEW_TOKEN_DECL( ComboboxUInt8, const std::string_view, uint8_t*, const char* const*, const size_t, const bool )
    NEW_TOKEN_DECL( Text, const std::string_view )
    NEW_TOKEN_DECL( Separator )
    NEW_TOKEN_DECL( SameLine )
    NEW_TOKEN_DECL( Table, std::string_view, size_t )
    NEW_TOKEN_DECL( TableRow )
    NEW_TOKEN_DECL( TableColumn, size_t )

    template <typename Numerical>
    struct ENGINE_CORE_API LabelAndNumericalToken
            : IUIToken<const std::string_view, Numerical &, float, Numerical, Numerical, bool>
    {
        using IUIToken<const std::string_view, Numerical &, float, Numerical, Numerical, bool>::IUIToken;
    };

#define NEW_LABEL_NUMERICAL_DECL(Name, Type) \
    struct ENGINE_CORE_API LabelAnd##Name##Token : LabelAndNumericalToken<##Type##> \
    { \
        using LabelAndNumericalToken<##Type##>::LabelAndNumericalToken; \
    };

    NEW_LABEL_NUMERICAL_DECL( Int, int )
    NEW_LABEL_NUMERICAL_DECL( Float, float )
    NEW_LABEL_NUMERICAL_DECL( UInt, uint32_t )
    NEW_LABEL_NUMERICAL_DECL( UInt16, uint16_t )
    NEW_LABEL_NUMERICAL_DECL( ULLD, uint64_t )
    NEW_TOKEN_DECL( LabelAndVec3, const std::string_view, float*, float, float, float, bool )
    NEW_TOKEN_DECL( LabelAndVec4, const std::string_view, float*, float, float, float, bool )
    NEW_TOKEN_DECL( LabelAndVec2, const std::string_view, float*, float, float, float, bool )
    NEW_TOKEN_DECL( DragAndDropSource, const std::string_view, const std::string_view, const void*, size_t )
    NEW_TOKEN_DECL( DragAndDropTarget, const std::string_view, const std::function<void(void* ptr)> )

    struct ENGINE_CORE_API UIContext
    {
        explicit UIContext(IUITokenBase* parent)
        {
	        m_parent_ = std::unique_ptr<IUITokenBase>(parent);
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
        IUITokenBase& operator<<(IUITokenBase* child)
        {
            m_parent_->AddChild(child);
            m_active_child_ = child;
	        return *m_parent_;
        }

        // Add Child and return this
        IUITokenBase& operator<=(IUITokenBase* child)
        {
            m_parent_->AddChild(child);
            m_active_child_ = child;
            return *child;
        }

        IUITokenBase& operator<<(const std::function<void()>& functor) const
        {
            m_parent_->SetFunction(functor);
	        return *m_parent_;
        }

        // Add Child and return child
        IUITokenBase& operator+=(IUITokenBase* child)
        {
            if (m_active_child_)
            {
                IUITokenBase* old_active = m_active_child_;
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
        IUITokenBase& operator|=(IUITokenBase* child) const
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

        IUITokenBase& operator+=(const std::function<void()>& functor) const
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

        IUITokenBase& operator>>(IUITokenBase* child) const
        {
            if (m_active_child_ && m_active_child_->GetParentInternal())
            {
				m_active_child_->GetParentInternal()->AddChild(child);
                return *child;
            }
            else
            {
	            m_parent_->AddChild(child);
                return *m_parent_;
            }
        }

        IUITokenBase& operator--()
        {
	        m_active_child_ = m_active_child_->GetParentInternal();
            return *m_active_child_;
        }

    private:
        std::unique_ptr<IUITokenBase> m_parent_;
        IUITokenBase* m_active_child_ = nullptr;
    };

#define TOKEN_PURE_GETTER_DECL(Name) \
    virtual IUITokenBase* New##Name##(const void* context, const std::string_view name, const Name##Token::ArgumentTuple& arguments) = 0;

    struct ENGINE_CORE_API IUIAPI
    {
        virtual ~IUIAPI() = default;

        [[nodiscard]] static UIContext NewContext(IUITokenBase* root)
        {
            return UIContext(root);
        }

        TOKEN_PURE_GETTER_DECL(MainMenuBar)
        TOKEN_PURE_GETTER_DECL(Menu)
        TOKEN_PURE_GETTER_DECL(MenuItem)
        TOKEN_PURE_GETTER_DECL(Dialog)
        TOKEN_PURE_GETTER_DECL(Button)
        TOKEN_PURE_GETTER_DECL(LabelAndText)
        TOKEN_PURE_GETTER_DECL(LabelAndFloat)
        TOKEN_PURE_GETTER_DECL(LabelAndInt)
        TOKEN_PURE_GETTER_DECL(LabelAndUInt)
        TOKEN_PURE_GETTER_DECL(LabelAndUInt16)
        TOKEN_PURE_GETTER_DECL(LabelAndULLD)
        TOKEN_PURE_GETTER_DECL(LabelAndPath)
        TOKEN_PURE_GETTER_DECL(ListBox)
        TOKEN_PURE_GETTER_DECL(TreeNode)
        TOKEN_PURE_GETTER_DECL(Selectable)
        TOKEN_PURE_GETTER_DECL(LabelAndVec3)
        TOKEN_PURE_GETTER_DECL(Checkbox)
        TOKEN_PURE_GETTER_DECL(Combobox)
        TOKEN_PURE_GETTER_DECL(LabelAndVec4)
        TOKEN_PURE_GETTER_DECL(DragAndDropSource)
        TOKEN_PURE_GETTER_DECL(DragAndDropTarget)
        TOKEN_PURE_GETTER_DECL(ComboboxUInt8)
        TOKEN_PURE_GETTER_DECL(Text)
        TOKEN_PURE_GETTER_DECL(Separator)
        TOKEN_PURE_GETTER_DECL(SameLine)
        TOKEN_PURE_GETTER_DECL(Table)
        TOKEN_PURE_GETTER_DECL(TableRow)
        TOKEN_PURE_GETTER_DECL(TableColumn)
		TOKEN_PURE_GETTER_DECL(LabelAndVec2)

        virtual void NewFrame() = 0;

    protected:
        template <typename T>
        T* Generate(const void* context, const std::string_view name, const typename T::ArgumentTuple& args)
        {
            return NewForwardTuple<T>(context, name, args, std::make_index_sequence<T::ArgumentCount::value>{});
        }

        template <typename T, typename Tuple, size_t... Is>
        T* NewForwardTuple(const void* context, const std::string_view name, const Tuple& t, std::index_sequence<Is...>)
        {
            return new T(context, name, std::get<Is>(t)...);
        }

        // todo: memory pool;
        std::tuple<> a;
    };

    struct ENGINE_CORE_API IUIAPIAccessor final
    {
        template <typename T> requires (std::is_base_of_v<IUIAPI, T>)
        void SetInterface()
        {
            if (!m_ui_interface_)
            {
                m_ui_interface_ = std::make_unique<T>();
            }
        }

        void NewFrame()
        {
            if (m_ui_interface_)
            {
                m_ui_interface_->NewFrame();
            }
        }

        [[nodiscard]] bool IsValid()
        {
            return m_ui_interface_ != nullptr;
        }

        [[nodiscard]] IUIAPI& GetInterface()
        {
            return *m_ui_interface_;
        }

        void Shutdown()
        {
	        if (IsValid())
	        {
		        m_ui_interface_.reset();
	        }
        }
        
    private:
        std::unique_ptr<IUIAPI> m_ui_interface_;
    };

    static IUIAPIAccessor s_uia;
}
