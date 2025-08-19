#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdint>
#include <cassert>


typedef void (*CustomWindowFunc)();


namespace FrameGUILayout
{

    class CustomLayoutNode
    {
    public:
        CustomLayoutNode(
            bool isLogicalDomain,
            uint32_t level,
            ImVec2 domainPos,
            ImVec2 domainSize,
            float splitterRatio,
            CustomWindowFunc customFunc = nullptr)
            : m_pLeft(nullptr),
              m_pRight(nullptr),
              m_level(level),
              m_domainPos(domainPos),
              m_domainSize(domainSize),
              m_splitterRatio(splitterRatio),
              m_splitterWidth(2.f),
              m_pfnCustomWindowFunc(customFunc),
              m_isLogicalDomain(isLogicalDomain)
        {}

        CustomLayoutNode(
            float splitterRatio)
            : m_splitterWidth(2.f),
              m_level(1),
              m_domainPos(ImVec2(0.f, 0.f)),
              m_domainSize(ImVec2(0.f, 0.f)),
              m_pfnCustomWindowFunc(nullptr),
              m_isLogicalDomain(true),
              m_pLeft(nullptr),
              m_pRight(nullptr),
              m_splitterRatio(splitterRatio)
        {}

        CustomLayoutNode(CustomWindowFunc customFunc)
            : m_splitterWidth(2.f),
              m_level(0),
              m_domainPos(ImVec2(0.f, 0.f)),
              m_domainSize(ImVec2(0.f, 0.f)),
              m_pfnCustomWindowFunc(customFunc),
              m_isLogicalDomain(false),
              m_pLeft(nullptr),
              m_pRight(nullptr),
              m_splitterRatio(0)
        {}

        ~CustomLayoutNode()
        {
            if (m_pLeft)
            {
                delete m_pLeft;
            }

            if (m_pRight)
            {
                delete m_pRight;
            }
        }

        CustomLayoutNode* GetLeftChild() const { return m_pLeft; }
        CustomLayoutNode* GetRightChild() const { return m_pRight; }
        ImVec2 GetDomainPos() const { return m_domainPos; }
        ImVec2 GetDomainSize() const { return m_domainSize; }
        uint32_t GetLevel() const { return m_level; }
        float GetSplitterStartCoord() const
        {
            if (m_level % 2 == 1)
            {
                return m_domainPos.x + m_splitterRatio * m_domainSize.x;
            }
            else
            {
                return m_domainPos.y + m_splitterRatio * m_domainSize.y;
            }
        }
        float GetSplitterWidth() const { return m_splitterWidth; }
        ImVec2 GetSplitterPos() const
        {
            if (m_level % 2 == 1)
            {
                float x = m_domainPos.x + m_splitterRatio * m_domainSize.x;
                return ImVec2(x, m_domainPos.y);
            }
            else
            {
                float y = m_domainPos.y + m_splitterRatio * m_domainSize.y;
                return ImVec2(m_domainPos.x, y);
            }
        }
        bool IsLogicalDomain() const { return m_isLogicalDomain; }

        void SetDomainPos(ImVec2 pos) { m_domainPos = pos; }
        void SetDomainSize(ImVec2 size) { m_domainSize = size; }
        void SetSplitterRatio(float ratio) { m_splitterRatio = ratio; }

        void CreateLeftChild(float ratio)
        {
            assert((void("ERROR: Only logical domain can have children."), m_isLogicalDomain == true));
            m_pLeft = new CustomLayoutNode(ratio);
            m_pLeft->m_level = m_level + 1;
        }

        void CreateLeftChild(CustomWindowFunc windowFunc)
        {
            assert((void("ERROR: Only logical domain can have children."), m_isLogicalDomain == true));
            m_pLeft = new CustomLayoutNode(windowFunc);
            m_pLeft->m_level = m_level + 1;
        }

        void CreateRightChild(float ratio)
        {
            assert((void("ERROR: Only logical domain can have children."), m_isLogicalDomain == true));
            m_pRight = new CustomLayoutNode(ratio);
            m_pRight->m_level = m_level + 1;
        }

        void CreateRightChild(CustomWindowFunc windowFunc)
        {
            assert((void("ERROR: Only logical domain can have children."), m_isLogicalDomain == true));
            m_pRight = new CustomLayoutNode(windowFunc);
            m_pRight->m_level = m_level + 1;
        }

        void BeginEndNodeAndChildren()
        {
            if (m_isLogicalDomain)
            {
                if (m_pLeft)
                {
                    m_pLeft->BeginEndNodeAndChildren();
                }

                if (m_pRight)
                {
                    m_pRight->BeginEndNodeAndChildren();
                }
            }
            else
            {
                ImGui::SetNextWindowPos(m_domainPos);
                ImGui::SetNextWindowSize(m_domainSize);
                if (m_pfnCustomWindowFunc)
                {
                    m_pfnCustomWindowFunc();
                }
            }
        }

        void ResizeNodeAndChildren(
            ImVec2 newPos,
            ImVec2 newSize)
        {

            m_domainPos = newPos;
            m_domainSize = newSize;

            if (m_isLogicalDomain)
            {
                float splitterStartCoordinate = GetSplitterStartCoord();
                float splitterWidth = GetSplitterWidth();
                if (m_level % 2 == 1)
                {
                    m_pLeft->ResizeNodeAndChildren(m_domainPos, ImVec2(splitterStartCoordinate - m_domainPos.x, m_domainSize.y));
                    m_pRight->ResizeNodeAndChildren(ImVec2(splitterStartCoordinate + splitterWidth, m_domainPos.y),
                        ImVec2(m_domainSize.x -
                            (splitterStartCoordinate - m_domainPos.x + splitterWidth),
                            m_domainSize.y));
                }
                else
                {
                    m_pLeft->ResizeNodeAndChildren(m_domainPos, ImVec2(m_domainSize.x,
                        splitterStartCoordinate - m_domainPos.y));
                    m_pRight->ResizeNodeAndChildren(ImVec2(m_domainPos.x, splitterStartCoordinate + splitterWidth),
                        ImVec2(m_domainSize.x,
                            m_domainSize.y -
                            (splitterStartCoordinate - m_domainPos.y + splitterWidth)));
                }
            }

        }

        CustomLayoutNode* GetHoverSplitter()
        {
            if (m_isLogicalDomain == false)
            {
                return nullptr;
            }
            ImVec2 splitterMin;
            ImVec2 splitterMax;

            constexpr float SplitterWidthPadding = 2.f;

            if (m_level % 2 == 1)
            {
                splitterMin.x = m_domainPos.x + m_splitterRatio * m_domainSize.x - SplitterWidthPadding;
                splitterMin.y = m_domainPos.y;
                splitterMax.x = splitterMin.x + m_splitterWidth + SplitterWidthPadding;
                splitterMax.y = m_domainPos.y + m_domainSize.y;
            }
            else
            {
                splitterMin.x = m_domainPos.x;
                splitterMin.y = m_domainPos.y + m_splitterRatio * m_domainSize.y - SplitterWidthPadding;
                splitterMax.x = m_domainPos.x + m_domainSize.x;
                splitterMax.y = splitterMin.y + m_splitterWidth + SplitterWidthPadding;
            }

            if (ImGui::IsMouseHoveringRect(splitterMin, splitterMax, false))
            {
                return this;
            }
            else
            {
                if (m_level % 2 == 1)
                {
                    // Left-Right
                    if (ImGui::IsMouseHoveringRect(m_domainPos, ImVec2(splitterMin.x, splitterMax.y), false))
                    {
                        return m_pLeft->GetHoverSplitter();
                    }
                    else
                    {
                        return m_pRight->GetHoverSplitter();
                    }
                }
                else
                {
                    // Top-Down
                    if (ImGui::IsMouseHoveringRect(m_domainPos, ImVec2(splitterMax.x, splitterMin.y), false))
                    {
                        return m_pLeft->GetHoverSplitter();
                    }
                    else
                    {
                        return m_pRight->GetHoverSplitter();
                    }
                }
            }
        }

    private:
        void SetLeftChild(CustomLayoutNode* pNode)
        {
            m_pLeft = pNode;
            m_pLeft->m_level = m_level + 1;
        }

        void SetRightChild(CustomLayoutNode* pNode)
        {
            m_pRight = pNode;
            m_pRight->m_level = m_level + 1;
        }


        CustomLayoutNode* m_pLeft;  // Or top for the even level splitters.
        CustomLayoutNode* m_pRight; // Or down for the even level splitters.
        uint32_t          m_level;  // Used to determine whether it is a vertical splitter or a horizontal splitter. 
                                    // Only for a splitter.
        ImVec2 m_domainPos;
        ImVec2 m_domainSize;

        float       m_splitterRatio; // (splitterPos - domainPos) / domainSize.
        const float m_splitterWidth;

        CustomWindowFunc m_pfnCustomWindowFunc;

        bool m_isLogicalDomain;
    };

    class CustomLayout
    {
    public:
        explicit CustomLayout(CustomLayoutNode* root)
            : m_pRoot(root),
              m_pHeldSplitterDomain(nullptr),
              m_splitterHeld(false),
              m_splitterBottonDownDelta(0.f),
              m_lastViewport(ImVec2(0.f, 0.f)),
              m_heldMouseCursor(0)
        {
            assert((void("ERROR: The root node pointer cannot be NULL to init CustomLayout."), root != nullptr));
        }
        
        void ResizeAll()
        {
            ImGuiViewport* pViewport = ImGui::GetMainViewport();

            if ((pViewport->WorkSize.x != m_lastViewport.x) || (pViewport->WorkSize.y != m_lastViewport.y))
            {
                m_pRoot->ResizeNodeAndChildren(pViewport->WorkPos, pViewport->WorkSize);
                m_lastViewport = pViewport->WorkSize;
            }
        }


        void BeginEndLayout()
        {
            ResizeAll();


            if (m_splitterHeld == false)
            {
                CustomLayoutNode* pSplitterDomain = m_pRoot->GetHoverSplitter();
                if (pSplitterDomain)
                {

                    bool isLeftRightSplitter = (pSplitterDomain->GetLevel() % 2 == 1);

                    isLeftRightSplitter ? ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW) :
                                          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);


                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        m_splitterHeld = true;

                        ImVec2 splitterPos = pSplitterDomain->GetSplitterPos();
                        ImVec2 mousePos = ImGui::GetMousePos();
                        m_splitterBottonDownDelta = isLeftRightSplitter ? splitterPos.x - mousePos.x :
                            splitterPos.y - mousePos.y;

                        m_pHeldSplitterDomain = pSplitterDomain;
                    }
                }
            }
            else
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    bool isLeftRightSplitter = (m_pHeldSplitterDomain->GetLevel() % 2 == 1);
                    ImVec2 domainPos = m_pHeldSplitterDomain->GetDomainPos();
                    ImVec2 domainSize = m_pHeldSplitterDomain->GetDomainSize();
                    ImVec2 mousePos = ImGui::GetMousePos();
                    float newSplitterRatio = -1.f;

                    if (isLeftRightSplitter)
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                        float newSplitterCoord = m_splitterBottonDownDelta + mousePos.x;
                        float newSplitterAxisLen = newSplitterCoord - domainPos.x;
                        newSplitterRatio = newSplitterAxisLen / domainSize.x;
                    }
                    else
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                        float newSplitterCoord = m_splitterBottonDownDelta + mousePos.y;
                        float newSplitterAxisLen = newSplitterCoord - domainPos.y;
                        newSplitterRatio = newSplitterAxisLen / domainSize.y;
                    }

                    m_pHeldSplitterDomain->SetSplitterRatio(newSplitterRatio);
                    m_pHeldSplitterDomain->ResizeNodeAndChildren(domainPos, domainSize);
                }
                else
                {
                    m_splitterHeld = false;
                }
            }


            if (m_pRoot != nullptr)
            {
                m_pRoot->BeginEndNodeAndChildren();
            }
        }

        ~CustomLayout()
        {
            if (m_pRoot)
            {
                delete m_pRoot;
            }
        }

        CustomLayoutNode* m_pRoot;

        bool  m_splitterHeld;
        float m_splitterBottonDownDelta;
        
        int   m_heldMouseCursor; 
        
        CustomLayoutNode* m_pHeldSplitterDomain;

        ImVec2 m_lastViewport;
    };

    bool BeginBottomMainMenuBar()
    {
        ImGuiContext& g = *GImGui;
        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();

        g.NextWindowData.MenuBarOffsetMinVal = ImVec2(g.Style.DisplaySafeAreaPadding.x, ImMax(g.Style.DisplaySafeAreaPadding.y - g.Style.FramePadding.y, 0.0f));
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();
        bool is_open = ImGui::BeginViewportSideBar("##BottomMainMenuBar", viewport, ImGuiDir_Down, height, window_flags);
        g.NextWindowData.MenuBarOffsetMinVal = ImVec2(0.0f, 0.0f);

        if (is_open)
            ImGui::BeginMenuBar();
        else
            ImGui::End();
        return is_open;
    }
}