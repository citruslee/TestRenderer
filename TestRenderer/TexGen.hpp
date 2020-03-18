#pragma once
#include "Renderer.hpp"
#include <vector>
#include <map>
#include <string>
#include <functional>

#include <unordered_set>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/ImNodesEzRokups.h"



namespace Generator
{

	/// A structure defining a connection between two slots of two nodes.
	struct FConnection
	{
		/// `id` that was passed to BeginNode() of input node.
		void* InputNode = nullptr;
		/// Descriptor of input slot.
		const char* InputSlot = nullptr;
		/// `id` that was passed to BeginNode() of output node.
		void* OutputNode = nullptr;
		/// Descriptor of output slot.
		const char* OutputSlot = nullptr;

		bool operator==(const FConnection& Other) const
		{
			return InputNode == Other.InputNode && InputSlot == Other.InputSlot && OutputNode == Other.OutputNode && OutputSlot == Other.OutputSlot;
		}

		bool operator!=(const FConnection& Other) const
		{
			return !operator ==(Other);
		}
	};

	enum ENodeSlotTypes
	{
		NodeSlotTexture = 1,
		NodeSlotFloat,
		NodeSlotFloat2,
		NodeSlotFloat4
	};

	enum class ENodeType
	{
		SCALAR,
		VECTOR2,
		VECTOR4,
		LOOP,
		RECTANGLE,
		SINE,
		ENVMAP,
		OUTPUT
	};

	/// A structure holding node state.
	struct SMyNode
	{
		/// Title which will be displayed at the center-top of the node.
		const char* Title = nullptr;
		/// Flag indicating that node is selected by the user.
		bool Selected = false;
		/// Node position on the canvas.
		ImVec2 Position{};
		/// List of node connections.
		std::vector<FConnection> Connections{};
		/// A list of input slots current node has.
		std::vector<ImNodes::Ez::SlotInfo> InputSlots{};
		/// A list of output slots current node has.
		std::vector<ImNodes::Ez::SlotInfo> OutputSlots{};

		ENodeType NodeType;

		explicit SMyNode(const char* Title, const ENodeType NodeType,
		                 const std::vector<ImNodes::Ez::SlotInfo>& InputSlots,
		                 const std::vector<ImNodes::Ez::SlotInfo>& OutputSlots);

		virtual EErrorCode Initialize(const FRenderer& Renderer) { return EErrorCode::OK; };
		virtual void Destroy(const FRenderer& Renderer) {};

		virtual void OnUpdate(float Time) {};
		virtual bool OnGui() { return false; }; // returns true if changed

		template<typename T>
		T* GetInputParameterNode(const char* InputName)
		{
			for (const auto& Connection : Connections)
			{
				if (std::string(Connection.InputSlot) == InputName)
				{
					return dynamic_cast<T*>((T*)Connection.OutputNode);
				}
			}

			return nullptr;
		}

		void CreateConnection(const char* InputSlot, SMyNode& OutputNode, const char* OutputSlot);

		/// Deletes connection from this node.
		void DeleteConnection(const FConnection& Connection);
	};

	struct SScalarNode : public SMyNode
	{
		SScalarNode()
			: SMyNode("Scalar", ENodeType::SCALAR, { }, {
					{ "Output", NodeSlotFloat },
				})
			, Value(0.0f)
		{
		}

		bool OnGui() override;

		float Value;
	};

	inline bool SScalarNode::OnGui()
	{
		ImGui::PushItemWidth(100);
		return ImGui::DragFloat("##ScalarNodeValueInput", &Value, 0.001f);
	}

	struct SVector2Node : public SMyNode
	{
		SVector2Node()
			: SMyNode("Vector2", ENodeType::VECTOR2, { }, {
					{ "Output", NodeSlotFloat2 },
				})
			, Value( 0.0f, 0.0f )
		{
			
		}

		bool OnGui() override
		{
			ImGui::PushItemWidth(100);
			return ImGui::DragFloat2("##Vector2NodeValueInput", reinterpret_cast<float *>(&Value), 0.001f);
		}

		DirectX::XMFLOAT2 Value;
	};

	struct SVector4Node : public SMyNode
	{
		SVector4Node()
			: SMyNode("Vector4", ENodeType::VECTOR4, { }, {
					{ "Output", NodeSlotFloat4 },
				})
			, Value( 0.0f, 0.0f, 0.0f, 0.0f )
		{
		}

		bool OnGui() override;

		DirectX::XMFLOAT4 Value;
	};

	struct STextureNode : public SMyNode
	{
		STextureNode(const wchar_t *ShaderName, const char* Title, const ENodeType NodeType, const std::vector<ImNodes::Ez::SlotInfo>& InputSlots, const std::vector<ImNodes::Ez::SlotInfo>& OutputSlots)
			: SMyNode(Title, NodeType, InputSlots, OutputSlots)
			, Renderer(nullptr)
			, ShaderName(ShaderName)
		{
		}

		bool OnGui() override;

		EErrorCode Initialize(const FRenderer& Renderer) override;

		void Destroy(const FRenderer& Renderer) override;

		void OnUpdate(float Time) override;

		SRenderTarget RenderTarget;

	protected:
		const FRenderer* Renderer;
		const wchar_t* ShaderName;
		SShader Shader{};
		SBuffer ConstantBuffer{};
	};

	

	struct SOutputNode : public STextureNode
	{
		SOutputNode()
			: STextureNode(nullptr, "Output", ENodeType::OUTPUT, {
				{ "Input", NodeSlotTexture }
				}, { })
		{
		}

		EErrorCode Initialize(const FRenderer& Renderer) override;

		void Destroy(const FRenderer& Renderer) override;

		void OnUpdate(float Time);
	};

	struct SRectangleNode : public STextureNode
	{
		struct SRectangleParams
		{
			DirectX::XMFLOAT4 Data; // X1 Y1 X2 Y2
			float Chamfer;
			float Falloff;
		};

		SRectangleNode()
			: STextureNode(L"Rectangle.hlsl", "Rectangle", ENodeType::RECTANGLE, {
						{ "Position", NodeSlotFloat4 },
						{ "Chamfer", NodeSlotFloat },
						{ "Falloff", NodeSlotFloat },
				}, {
					{"Output", NodeSlotTexture}
				})
		{
		}

		void OnUpdate(float Time) override;

		SRectangleParams Params{};
	};

	struct SLoopNode : public STextureNode
	{
		struct SLoopParams
		{
			DirectX::XMFLOAT2 Repeat;
		};

		SLoopNode()
			: STextureNode(L"Loop.hlsl", "Loop", ENodeType::LOOP, {
				               {"Repeat", NodeSlotFloat2},
				               {"Input", NodeSlotTexture},
			               }, {
				               {"Output", NodeSlotTexture}
			               }), Params()
		{
		}

		void OnUpdate(float Time) override;

		SLoopParams Params;
	};

	struct SSineDist : public STextureNode
	{
		struct SSineDistParams
		{
			float CountX;
			float AmplX;
			float CountY;
			float AmplY;
		};

		explicit SSineDist()
			: STextureNode(L"SineDist.hlsl", "SineDist", ENodeType::SINE, {
					{ "Count", NodeSlotFloat2 },
					{ "Amplitude", NodeSlotFloat2 },
					{ "Input", NodeSlotTexture }
				}, {
					{ "Output", NodeSlotTexture }
				})
		{
		}

		void OnUpdate(float Time) override;

		SSineDistParams Params{};
	};

	class FTexGen
	{
	public:

		explicit FTexGen(FRenderer& Renderer);

		~FTexGen();

		EErrorCode Initialize(const uint32_t Width, const uint32_t Height);

		void OnUpdate(const float Time) noexcept;

		// topologically traverse nodegraph and execute function for each node
		bool Visit(std::function<bool(SMyNode* node)> Callback) noexcept;

		SRenderTarget GetOutput() const;

		bool IsOutputUpdated();

		void OnGui() noexcept;

	private:
		bool bIsDirty;
		bool bIsOutputUpdated;
		FRenderer& InternalRenderer;
		SOutputNode* GraphEntryPoint;

		std::map<std::string, SMyNode* (*)()> AvailableNodes{
			{
				"Scalar", []() -> SMyNode *
				{
					return new SScalarNode();
				}
			},
			{
				"Vector2", []() -> SMyNode*
				{
					return new SVector2Node();
				}
			},
			{
				"Vector4", []() -> SMyNode*
				{
					return new SVector4Node();
				}
			},
			{
				"Loop", []() -> SMyNode *
				{
					return new SLoopNode();
				}
			},
			{
				"Rectangle", []() -> SMyNode*
				{
					return new SRectangleNode();
				}
			},
			{
				"SineDist", []() -> SMyNode*
				{
					return new SSineDist();
				}
			},
			{
				"Output", []() -> SMyNode*
				{
					return new SOutputNode();
				}
			},
		};
		std::vector<SMyNode*> Nodes;
	};
}
