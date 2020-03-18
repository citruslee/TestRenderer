#include "TexGen.hpp"
#include <stack>

#define NODE_INPUT1(VariableType, Variable, NodeType, InputName, Statement, DefaultValue) \
VariableType Variable; \
{ \
	auto Node = GetInputParameterNode<NodeType>(#InputName); \
	if (Node) \
	{ \
		Variable = (Statement); \
	} \
	else \
	{ \
		Variable = DefaultValue; \
	} \
}

#define NODE_INPUT2(Variable, NodeType, InputName, Statement, DefaultValue) \
{ \
	auto Node = GetInputParameterNode<NodeType>(#InputName); \
	if (Node) \
	{ \
		Variable = (Statement); \
	} \
	else \
	{ \
		Variable = DefaultValue; \
	} \
}

Generator::SMyNode::SMyNode(const char* Title, const ENodeType NodeType,
                            const std::vector<ImNodes::Ez::SlotInfo>& InputSlots,
                            const std::vector<ImNodes::Ez::SlotInfo>& OutputSlots)
{
	this->Title = Title;
	this->NodeType = NodeType;
	this->InputSlots = InputSlots;
	this->OutputSlots = OutputSlots;
}

void Generator::SMyNode::CreateConnection(const char* InputSlot, SMyNode& OutputNode, const char* OutputSlot)
{
	FConnection NewConnection;
	NewConnection.InputNode = this;
	NewConnection.InputSlot = InputSlot;
	NewConnection.OutputNode = &OutputNode;
	NewConnection.OutputSlot = OutputSlot;
	static_cast<SMyNode*>(NewConnection.InputNode)->Connections.push_back(NewConnection);
	static_cast<SMyNode*>(NewConnection.OutputNode)->Connections.push_back(NewConnection);
}

void Generator::SMyNode::DeleteConnection(const FConnection& Connection)
{
	for (auto Iterator = Connections.begin(); Iterator != Connections.end(); ++Iterator)
	{
		if (Connection == *Iterator)
		{
			Connections.erase(Iterator);
			break;
		}
	}
}

EErrorCode Generator::SOutputNode::Initialize(const FRenderer& Renderer)
{
	return EErrorCode::OK;
}

void Generator::SOutputNode::Destroy(const FRenderer& Renderer)
{
}

void Generator::SOutputNode::OnUpdate(float Time)
{
	// copy last render target to self - mark as ouput
	NODE_INPUT2(RenderTarget, STextureNode, Input, Node->RenderTarget, {});
}

bool Generator::SVector4Node::OnGui()
{
	ImGui::PushItemWidth(200);
	return ImGui::DragFloat4("##Vector4NodeValueInput", reinterpret_cast<float *>(&Value), 0.001f);
}

bool Generator::STextureNode::OnGui()
{
	const auto Size = 100;
	ImGui::BeginChild("##ImageChild", ImVec2(Size + 16, Size + 16), true,
	                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::Image(RenderTarget.ShaderResourceView, ImVec2(Size, Size));
	ImGui::EndChild();
	return false;
}

EErrorCode Generator::STextureNode::Initialize(const FRenderer& Renderer)
{
	char Dummy[128] = { 0 };
	auto Result = Renderer.CreateVertexShader(L"FullScreenTriangleVS.hlsl", "main", nullptr, 0, Shader);
	Result = Renderer.CreatePixelShader(ShaderName, "main", Shader);
	Result = Renderer.CreateRenderTarget(1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, RenderTarget);
	Result = Renderer.CreateConstantBufferWithData(Dummy, ConstantBuffer);
	this->Renderer = &Renderer;
	return EErrorCode::OK;
}

void Generator::STextureNode::Destroy(const FRenderer& Renderer)
{
	Renderer.DestroyShader(Shader);
	Renderer.DestroyBuffer(ConstantBuffer);
	//Renderer.DestroyRenderTarget(RenderTarget);
}

void Generator::STextureNode::OnUpdate(float Time)
{
	Renderer->SetRenderTarget(RenderTarget);
	Renderer->ClearRenderTarget(RenderTarget, DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
	Renderer->SetViewport(RenderTarget.Width, RenderTarget.Height);
	Renderer->SetShader(Shader);
	Renderer->SetConstantBuffer(ConstantBuffer, EShaderStage::PIXEL);
	Renderer->SetConstantBuffer({nullptr, 0}, EShaderStage::VERTEX);
	Renderer->SetVertexBuffer(0, {nullptr, 0}, 0);
	Renderer->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Renderer->Draw(3, 0);
	Renderer->UnbindRenderTargets();
}

void Generator::SRectangleNode::OnUpdate(float Time)
{
	NODE_INPUT2(Params.Data, SVector4Node, Position, Node->Value, DirectX::XMFLOAT4(0.5f, 0.5f, 1.0f, 1.0f));
	NODE_INPUT2(Params.Chamfer, SScalarNode, Chamfer, Node->Value, 3.0f);
	NODE_INPUT2(Params.Falloff, SScalarNode, Falloff, Node->Value, 0.0f);
	Renderer->UpdateSubresource(ConstantBuffer, &Params, sizeof(Params));
	STextureNode::OnUpdate(Time);
}

void Generator::SLoopNode::OnUpdate(float Time)
{
	NODE_INPUT2(Params.Repeat, SVector2Node, Repeat, Node->Value, DirectX::XMFLOAT2(1.0f, 1.0f));
	NODE_INPUT1(SRenderTarget, InputTexture, SRectangleNode, Input, Node->RenderTarget, {});

	Renderer->SetTexture(0, InputTexture);
	Renderer->UpdateSubresource(ConstantBuffer, &Params, sizeof(Params));
	STextureNode::OnUpdate(Time);
}

void Generator::SSineDist::OnUpdate(float Time)
{
	NODE_INPUT1(DirectX::XMFLOAT2, Count, SVector2Node, Count, Node->Value, {});
	NODE_INPUT1(DirectX::XMFLOAT2, Amplitude, SVector2Node, Amplitude, Node->Value, {});
	NODE_INPUT1(SRenderTarget, InputTexture, STextureNode, Input, Node->RenderTarget, {});

	Params.CountX = Count.x;
	Params.CountY = Count.y;
	Params.AmplX = Amplitude.x;
	Params.AmplY = Amplitude.y;

	Renderer->SetTexture(0, InputTexture);
	Renderer->UpdateSubresource(ConstantBuffer, &Params, sizeof(Params));
	STextureNode::OnUpdate(Time);
}

Generator::FTexGen::FTexGen(FRenderer& Renderer): InternalRenderer(Renderer)
{
}

Generator::FTexGen::~FTexGen()
{
	for (auto& Node : Nodes)
	{
		Node->Destroy(InternalRenderer);
	}
}

EErrorCode Generator::FTexGen::Initialize(const uint32_t Width, const uint32_t Height)
{
	auto Node1 = new SVector4Node();
	Node1->Position.x = -300;
	Node1->Position.y = 350;
	Node1->Value.x = 0.75f;
	Node1->Value.y = 0.5f;
	Node1->Value.z = 0.5f;
	Node1->Value.w = 1.0f;
	Nodes.push_back(Node1);

	auto Node2 = new SScalarNode();
	Node2->Position.x = -300;
	Node2->Position.y = 450;
	Nodes.push_back(Node2);

	auto Node3 = new SRectangleNode();
	Node3->Position.x = 100;
	Node3->Position.y = 400;
	Nodes.push_back(Node3);

	auto Node4 = new SOutputNode();
	Node4->Position.x = 500;
	Node4->Position.y = 400;
	Nodes.push_back(Node4);

	Node3->CreateConnection("Position", *Node1, "Output");
	Node3->CreateConnection("Chamfer", *Node2, "Output");
	Node4->CreateConnection("Input", *Node3, "Output");

	for (auto& Node : Nodes)
	{
		Node->Initialize(InternalRenderer);
	}

	GraphEntryPoint = dynamic_cast<SOutputNode*>(Node4);
	bIsDirty = true;
	return EErrorCode::OK;
}

void Generator::FTexGen::OnUpdate(const float Time) noexcept
{
	// generate texture only if graph change
	if (!bIsDirty)
	{
		return;
	}

	auto Result = Visit([Time](SMyNode* Node)
	{
		Node->OnUpdate(Time);
		return true;
	});

	if (Result)
	{
		bIsOutputUpdated = true;
	}

	bIsDirty = false;
}

bool Generator::FTexGen::Visit(std::function<bool(SMyNode* node)> Callback) noexcept
{
	std::stack<SMyNode*> Stack;
	std::unordered_set<SMyNode*> Visited;
	if (!GraphEntryPoint)
	{
		return false;
	}

	Stack.push(GraphEntryPoint);
	while (!Stack.empty())
	{
		auto Node = Stack.top();
		if (Visited.find(Node) == Visited.end())
		{
			Visited.insert(Node);

			// read all children
			for (const auto& Connection : Node->Connections)
			{
				Stack.push(static_cast<SMyNode*>(Connection.OutputNode));
			}

			if (Node->Connections.empty())
			{
				goto ProcessNode;
			}
		}
		else
		{
		ProcessNode:
			Stack.pop();
			Callback(Node);
		}
	}
	return true;
}

SRenderTarget Generator::FTexGen::GetOutput() const
{
	return GraphEntryPoint ? GraphEntryPoint->RenderTarget : SRenderTarget();
}

bool Generator::FTexGen::IsOutputUpdated()
{
	auto B = bIsOutputUpdated;
	bIsOutputUpdated = false;
	return B;
}

void Generator::FTexGen::OnGui() noexcept
{
	// Canvas must be created after ImGui initializes, because constructor accesses ImGui style to configure default colors.
	static ImNodes::CanvasState Canvas{};

	const ImGuiStyle& Style = ImGui::GetStyle();
	if (ImGui::Begin("Texture Generator", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		// We probably need to keep some state, like positions of nodes/slots for rendering connections.
		ImNodes::BeginCanvas(&Canvas);
		for (auto Iterator = Nodes.begin(); Iterator != Nodes.end();)
		{
			SMyNode* Node = *Iterator;

			// Start rendering node
			if (ImNodes::Ez::BeginNode(Node, Node->Title, &Node->Position, &Node->Selected))
			{
				// Render input nodes first (order is important)
				ImNodes::Ez::InputSlots(Node->InputSlots.data(), Node->InputSlots.size());

				// process node custom gui
				if (Node->OnGui())
				{
					bIsDirty = true;
				}

				// Render output nodes first (order is important)
				ImNodes::Ez::OutputSlots(Node->OutputSlots.data(), Node->OutputSlots.size());

				// Store new connections when they are created
				FConnection NewConnection;
				if (ImNodes::GetNewConnection(&NewConnection.InputNode, &NewConnection.InputSlot,
				                              &NewConnection.OutputNode, &NewConnection.OutputSlot))
				{
					static_cast<SMyNode*>(NewConnection.InputNode)->Connections.push_back(NewConnection);
					static_cast<SMyNode*>(NewConnection.OutputNode)->Connections.push_back(NewConnection);
					bIsDirty = true;
				}

				// Render output connections of this node
				for (const FConnection& Connection : Node->Connections)
				{
					// Node contains all it's connections (both from output and to input slots). This means that multiple
					// nodes will have same connection. We render only output connections and ensure that each connection
					// will be rendered once.
					if (Connection.OutputNode != Node)
					{
						continue;
					}
					if (!ImNodes::Connection(Connection.InputNode, Connection.InputSlot, Connection.OutputNode,
					                         Connection.OutputSlot))
					{
						// Remove deleted connections
						static_cast<SMyNode*>(Connection.InputNode)->DeleteConnection(Connection);
						static_cast<SMyNode*>(Connection.OutputNode)->DeleteConnection(Connection);
						bIsDirty = true;
					}
				}
			}
			// Node rendering is done. This call will render node background based on size of content inside node.
			ImNodes::Ez::EndNode();

			if (Node->Selected && ImGui::IsKeyPressedMap(ImGuiKey_Delete))
			{
				// Deletion order is critical: first we delete connections to us
				for (auto& Connection : Node->Connections)
				{
					if (Connection.OutputNode == Node)
					{
						static_cast<SMyNode*>(Connection.InputNode)->DeleteConnection(Connection);
					}
					else
					{
						static_cast<SMyNode*>(Connection.OutputNode)->DeleteConnection(Connection);
					}
				}
				// Then we delete our own connections, so we don't corrupt the list
				Node->Connections.clear();
				if (Node == GraphEntryPoint)
				{
					GraphEntryPoint = nullptr;
				}

				delete Node;
				Iterator = Nodes.erase(Iterator);
				bIsDirty = true;
			}
			else
			{
				++Iterator;
			}
		}

		const ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered() && !ImGui::IsMouseDragging(1))
		{
			ImGui::FocusWindow(ImGui::GetCurrentWindow());
			ImGui::OpenPopup("NodesContextMenu");
		}

		if (ImGui::BeginPopup("NodesContextMenu"))
		{
			for (const auto& Desc : AvailableNodes)
			{
				if (ImGui::MenuItem(Desc.first.c_str()))
				{
					auto Node = Desc.second();
					Node->Initialize(InternalRenderer);
					Nodes.push_back(Node);
					ImNodes::AutoPositionNode(Nodes.back());

					if (Node->NodeType == ENodeType::OUTPUT)
					{
						GraphEntryPoint = dynamic_cast<SOutputNode *>(Nodes.back());
					}
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Reset Zoom"))
			{
				Canvas.zoom = 1;
			}
			if (ImGui::IsAnyMouseDown() && !ImGui::IsWindowHovered())
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImNodes::EndCanvas();
	}
	ImGui::End();
}

#undef NODE_INPUT1
#undef NODE_INPUT2