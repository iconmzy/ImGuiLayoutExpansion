#pragma once


#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>


using CustomWindowFunc = void (*)();

namespace FrameGUILayout {

	class CustomLayoutNode {
	public:
		CustomLayoutNode(bool isVertical, const char* label = nullptr);
		CustomLayoutNode(CustomWindowFunc func, const char* label);
		~CustomLayoutNode();

		void AddVerticalChild(CustomLayoutNode* child);
		void AddHorizontalChild(CustomLayoutNode* child);
		void SetHorizontalChildren(CustomLayoutNode* left, CustomLayoutNode* middle = nullptr, CustomLayoutNode* right = nullptr);
		void SetVerticalChildren(CustomLayoutNode* left, CustomLayoutNode* middle = nullptr, CustomLayoutNode* right = nullptr);

		const std::string& GetLabel() const;
		void SetLabel(const char* label);

		void SetVisible(bool v);
		bool IsVisibleFlag() const;
		bool IsEffectivelyVisible() const;
		void SetDFSVisible(bool new_status);

		void ResizeNodeAndChildren(ImVec2 newPos, ImVec2 newSize);
		void RenderNodeAndChildren();
		bool FindHoveredSplitter(const ImVec2& mousePos, CustomLayoutNode*& outNode, int& outBoundaryIndex);
		bool HandleSplitterDragAt(int boundaryIndex, const ImVec2& mouseDelta);

		int VisibleChildCount() const;
		bool IsWindowNode() const;
		bool IsVerticalSplitter() const;
		bool IsHorizontalSplitter() const;

		const std::vector<CustomLayoutNode*>& GetChildren() const;
		std::vector<CustomLayoutNode*>& GetChildren();

		void EqualizeIfVisibleCountChanged();
		ImVec2 GetDomainPos() const;
		ImVec2 GetDomainSize() const;

	private:
		bool m_isVertical = false;
		std::string m_label;
		bool m_visible = true;
		ImVec2 m_domainPos{ 0,0 };
		ImVec2 m_domainSize{ 0,0 };
		std::vector<CustomLayoutNode*> m_children;
		std::vector<float> m_splitRatios;
		float m_splitterWidth = 4.0f;
		float m_minRatio = 0.05f;
		size_t m_lastVisibleCount = 0;
		bool m_equalizeOnVisibleChange = true;
		CustomWindowFunc m_pfnCustomWindowFunc = nullptr;

		static inline float Clamp01(float v);
	};

	class CustomLayout {
	public:
		explicit CustomLayout(CustomLayoutNode* root);
		~CustomLayout();

		void UpdateAndRender();
		CustomLayoutNode* GetRoot();

	private:
		void DrawControlPanel();

		CustomLayoutNode* m_root = nullptr;
		ImVec2 m_lastViewportSize{ 0,0 };
		CustomLayoutNode* m_activeNode = nullptr;
		int m_activeBoundaryIndex = -1;
	};

} // namespace FrameGUILayout