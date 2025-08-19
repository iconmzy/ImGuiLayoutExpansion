#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdint>
#include <cassert>
#include <vector>

typedef void (*CustomWindowFunc)();


namespace FrameGUILayout
{

    class CustomLayoutNode
    {
    public:
		CustomLayoutNode(
			bool isVertical,
			ImVec2 domainPos,
			ImVec2 domainSize,
			const std::vector<float>& splitRatios = {},
			CustomWindowFunc customFunc = nullptr
		)
			: m_isVertical(isVertical),
			m_domainPos(domainPos),
			m_domainSize(domainSize),
			m_splitRatios(splitRatios),
			m_splitterWidth(4.0f),
			m_pfnCustomWindowFunc(customFunc)
		{
			if (isVertical) {
				assert(!splitRatios.empty() && "Vertical layout must have at least one split ratio");
			}
		}

		
		CustomLayoutNode(CustomWindowFunc customFunc)
			: m_isVertical(false),
			m_domainPos(ImVec2(0, 0)),
			m_domainSize(ImVec2(0, 0)),
			m_splitterWidth(0),
			m_pfnCustomWindowFunc(customFunc)
		{
		}

		~CustomLayoutNode() {
			for (auto* child : m_children) {
				delete child;
			}
		}

		void AddVerticalChild(CustomLayoutNode* child) {
			assert(m_isVertical && "Only vertical nodes can have dynamic children");
			m_children.push_back(child);
			if (m_splitRatios.empty()) {
				m_splitRatios.push_back(1.0f);
			}
		}
		void SetHorizontalChildren(CustomLayoutNode* left, CustomLayoutNode* middle = nullptr, CustomLayoutNode* right = nullptr) {
			assert(!m_isVertical && "Horizontal nodes can only have fixed children");
			m_children.clear();
			if (left) m_children.push_back(left);
			if (middle) m_children.push_back(middle);
			if (right) m_children.push_back(right);
			assert(m_children.size() <= 3 && "Horizontal nodes support max 3 children");
		}

		
		void ResizeNodeAndChildren(ImVec2 newPos, ImVec2 newSize) {
			m_domainPos = newPos;
			m_domainSize = newSize;

			if (IsWindowNode()) return;

			if (m_isVertical) {
				float totalHeight = m_domainSize.y;
				float accumulatedY = m_domainPos.y;

				for (size_t i = 0; i < m_children.size(); ++i) {
					float ratio = (i < m_splitRatios.size()) ? m_splitRatios[i] : 1.0f;
					float childHeight = totalHeight * ratio;

					m_children[i]->ResizeNodeAndChildren(
						ImVec2(m_domainPos.x, accumulatedY),
						ImVec2(m_domainSize.x, childHeight)
					);
					accumulatedY += childHeight;
				}
			}
			else {
				float childWidth = m_domainSize.x / m_children.size();
				for (size_t i = 0; i < m_children.size(); ++i) {
					m_children[i]->ResizeNodeAndChildren(
						ImVec2(m_domainPos.x + i * childWidth, m_domainPos.y),
						ImVec2(childWidth, m_domainSize.y)
					);
				}
			}
		}

      
        ImVec2 GetDomainPos() const { return m_domainPos; }
        ImVec2 GetDomainSize() const { return m_domainSize; }
		void SetDomainPos(ImVec2 pos) { m_domainPos = pos; }
		void SetDomainSize(ImVec2 size) { m_domainSize = size; }
		float GetSplitterWidth() const { return m_splitterWidth; }
		const std::vector<float>& GetSplitRatios() const { return m_splitRatios; }
		void SetSplitRatios(const std::vector<float>& ratios) {
			assert(m_isVertical || ratios.size() <= 3);
			m_splitRatios = ratios;
		}
		bool IsVertical() const { return m_isVertical; }
		bool IsWindowNode() const { return m_pfnCustomWindowFunc != nullptr; }
		bool IsVerticalSplitter() const { return m_isVertical && !IsWindowNode(); }
		bool IsHorizontalSplitter() const { return !m_isVertical && !IsWindowNode(); }
		const std::vector<CustomLayoutNode*>& GetChildren() const { return m_children; }



        //render function 


		void RenderNodeAndChildren() {
			if (IsWindowNode()) {
				ImGui::SetNextWindowPos(m_domainPos);
				ImGui::SetNextWindowSize(m_domainSize);
				ImGui::Begin("Window", nullptr,
					ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_NoCollapse);

				if (m_pfnCustomWindowFunc) {
					m_pfnCustomWindowFunc();
				}

				ImGui::End();
				return;
			}

			if (m_isVertical) {
				for (size_t i = 0; i < m_children.size() - 1; ++i) {
					float splitterY = m_domainPos.y;
					for (size_t j = 0; j <= i; ++j) {
						splitterY += m_domainSize.y * m_splitRatios[j];
					}

					ImGui::GetWindowDrawList()->AddLine(
						ImVec2(m_domainPos.x, splitterY),
						ImVec2(m_domainPos.x + m_domainSize.x, splitterY),
						IM_COL32(100, 100, 100, 255),
						m_splitterWidth
					);
				}
			}
			else {
				for (size_t i = 1; i < m_children.size(); ++i) {
					float splitterX = m_domainPos.x + (m_domainSize.x / m_children.size()) * i;

					ImGui::GetWindowDrawList()->AddLine(
						ImVec2(splitterX, m_domainPos.y),
						ImVec2(splitterX, m_domainPos.y + m_domainSize.y),
						IM_COL32(100, 100, 100, 255),
						m_splitterWidth
					);
				}
			}

			for (auto* child : m_children) {
				child->RenderNodeAndChildren();
			}
		}
		
        // Interaction function
		CustomLayoutNode* GetHoveredSplitter(const ImVec2& mousePos) {
			if (IsWindowNode()) return nullptr;

			const float hitPadding = 8.0f; 

			if (m_isVertical) {
				
				float accumulatedY = m_domainPos.y;
				for (size_t i = 0; i < m_children.size() - 1; ++i) {
					accumulatedY += m_domainSize.y * m_splitRatios[i];
					if (mousePos.y >= accumulatedY - hitPadding &&
						mousePos.y <= accumulatedY + hitPadding) {
						return this;
					}
				}
			}
			else {
				float childWidth = m_domainSize.x / m_children.size();
				for (size_t i = 1; i < m_children.size(); ++i) {
					float splitterX = m_domainPos.x + childWidth * i;
					if (mousePos.x >= splitterX - hitPadding &&
						mousePos.x <= splitterX + hitPadding) {
						return this;
					}
				}
			}

			for (auto* child : m_children) {
				if (auto* hovered = child->GetHoveredSplitter(mousePos)) {
					return hovered;
				}
			}

			return nullptr;
		}

		bool HandleSplitterDrag(CustomLayoutNode* activeSplitter, const ImVec2& mouseDelta) {
			if (this != activeSplitter) return false;

			if (m_isVertical) {
				
				float totalHeight = m_domainSize.y;
				float deltaRatio = mouseDelta.y / totalHeight;

				for (size_t i = 0; i < m_splitRatios.size(); ++i) {
					if (i == 0) {
						m_splitRatios[i] += deltaRatio;
						m_splitRatios[i] = ImClamp(m_splitRatios[i], 0.1f, 0.9f);
					}
					else {
						m_splitRatios[i] -= deltaRatio;
						m_splitRatios[i] = ImClamp(m_splitRatios[i], 0.1f, 0.9f);
					}
				}
			}
			else {
				
				float totalWidth = m_domainSize.x;
				float deltaRatio = mouseDelta.x / totalWidth;

				
				if (m_children.size() == 2) {
					m_splitRatios[0] += deltaRatio;
					m_splitRatios[0] = ImClamp(m_splitRatios[0], 0.1f, 0.9f);
					m_splitRatios[1] = 1.0f - m_splitRatios[0];
				}
		
			}

			
			ResizeNodeAndChildren(m_domainPos, m_domainSize);
			return true;
		}
       
       

    private:
		bool m_isVertical;
		ImVec2 m_domainPos;
		ImVec2 m_domainSize;
		std::vector<float> m_splitRatios;
		const float m_splitterWidth;
		CustomWindowFunc m_pfnCustomWindowFunc;
		std::vector<CustomLayoutNode*> m_children;
    };

    class CustomLayout
    {
	private:
		CustomLayoutNode* m_pActiveSplitter = nullptr;
		ImVec2 m_lastViewportSize;

    public:
        explicit CustomLayout(CustomLayoutNode* root)
            : m_pRoot(root),
              m_pHeldSplitterDomain(nullptr),
              m_splitterHeld(false),
              m_splitterBottonDownDelta(0.f),
              m_lastViewport(ImVec2(0.f, 0.f)),
              m_heldMouseCursor(0)
        {
            assert((void("ERROR: The root node pointer cannot be NULL to init FrameGUILayout."), root != nullptr));
        }
        
		~CustomLayout()
		{
			if (m_pRoot)
			{
				delete m_pRoot;
			}
		}

		void UpdateAndRender() {
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			if (viewport->WorkSize.x != m_lastViewportSize.x ||
				viewport->WorkSize.y != m_lastViewportSize.y) {
				m_pRoot->ResizeNodeAndChildren(viewport->WorkPos, viewport->WorkSize);
				m_lastViewportSize = viewport->WorkSize;
			}


			ImVec2 mousePos = ImGui::GetMousePos();

			if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				m_pActiveSplitter = m_pRoot->GetHoveredSplitter(mousePos);
			}

			if (m_pActiveSplitter) {
				ImGui::SetMouseCursor(m_pActiveSplitter->IsVertical() ?
					ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
			}

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				if (m_pActiveSplitter) {
					ImVec2 delta = ImGui::GetMouseDragDelta();
					m_pActiveSplitter->HandleSplitterDrag(m_pActiveSplitter, delta);
					ImGui::ResetMouseDragDelta();
				}
			}


			m_pRoot->RenderNodeAndChildren();
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