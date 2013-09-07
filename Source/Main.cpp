#include "CoreEvents.h"
#include "Engine.h"
#include "Font.h"
#include "Input.h"
#include "ProcessUtils.h"
#include "Text.h"
#include "UI.h"
#include "ResourceCache.h"
#include "Script.h"
#include "ScriptFile.h"
#include "Camera.h"
#include "Octree.h"
#include "StaticModel.h"
#include "Model.h"
#include "Material.h"
#include "Console.h"
#include "Renderer.h"
#include "PhysicsEvents.h"
#include "RigidBody.h"
#include "FileSystem.h"

#include "Main.h"

/// manage in-game scene editor
class Editor : public Urho3D::RefCounted
{
    public: 
        Editor(Urho3D::Context* context)
            : context_(context)
        {
        }

        void Start()
        {
            editorScript_ = context_->GetSubsystem<Urho3D::ResourceCache>()->
                    GetResource<Urho3D::ScriptFile>("Scripts/Editor.as");
            if (editorScript_)
                editorScript_->Execute("void Start()");
        }

        void Stop()
        {
            if (editorScript_)
                editorScript_->Execute("void Stop()");
        }

    private:
        Urho3D::Context* context_;
        Urho3D::SharedPtr<Urho3D::ScriptFile> editorScript_;
};



/// our player component
class Player : public Urho3D::Component
{
    OBJECT(Player);
    
    public:
        /// construct
        Player(Urho3D::Context* context)
            : Component(context)
        {
        }
        
    protected:
        /// handle node being assigned
        virtual void OnNodeSet(Urho3D::Node* node)
        {
            if (node)
                SubscribeToEvent(Urho3D::E_UPDATE, HANDLER(Player, Update));
        }
        
    private:
        /// per-frame update event handler
        void Update(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
        {
            float dt = eventData[Urho3D::Update::P_TIMESTEP].GetFloat();

            Urho3D::Input *input = GetSubsystem<Urho3D::Input>();
            if (input->GetKeyDown('J'))
                GetComponent<Urho3D::RigidBody>()->ApplyForce(
                        dt * Urho3D::Vector3(0, 2000, 0)
                        );
        }
};



DEFINE_APPLICATION_MAIN(Main) // entry point

Main::Main(Urho3D::Context* context) :
    Application(context)
{
    context->RegisterFactory<Player>("Game");
}

void Main::Setup()
{
    // defaults
    launchEditor_                    = false;
    engineParameters_["WindowTitle"] = GetTypeName();
    engineParameters_["LogName"]     = GetTypeName() + ".log";
    engineParameters_["FullScreen"]  = false;
    engineParameters_["Headless"]    = false;

    // read commandline arguments
    const Urho3D::Vector<Urho3D::String>& arguments = Urho3D::GetArguments();
    for (unsigned i = 0; i < arguments.Size(); ++i)
        if (arguments[i] == "-edit")
            launchEditor_ = true;
}

void Main::Start()
{
    // initialize script subsystem
    context_->RegisterSubsystem(new Urho3D::Script(context_));

    // create console
    Urho3D::XMLFile* xmlFile = GetSubsystem<Urho3D::ResourceCache>()->
        GetResource<Urho3D::XMLFile>("UI/DefaultStyle.xml");
    if (!xmlFile)
        return;
    Urho3D::Console* console = engine_->CreateConsole();
    console->SetDefaultStyle(xmlFile);

    // bind events
    SubscribeToEvent(Urho3D::E_UPDATE, HANDLER(Main, Update));
    SubscribeToEvent(Urho3D::E_KEYDOWN, HANDLER(Main, KeyDown));

    // load editor/scene
    if (launchEditor_)
    {
        editor_ = new Editor(context_);
        editor_->Start();
    }
    else
    {
        // scene
        scene_ = new Urho3D::Scene(context_);

        // camera
        cameraNode_ = new Urho3D::Node(context_);
        cameraNode_->CreateComponent<Urho3D::Camera>();
        cameraNode_->SetPosition(Urho3D::Vector3(0.0f, 5.0f, 0.0f));

        // viewport
        Urho3D::Renderer* renderer = GetSubsystem<Urho3D::Renderer>();
        Urho3D::SharedPtr<Urho3D::Viewport> viewport(new Urho3D::Viewport(context_,
                    scene_, cameraNode_->GetComponent<Urho3D::Camera>()));
        renderer->SetViewport(0, viewport);
    }
}

void Main::Stop()
{
    if (editor_)
        editor_->Stop();
}


void Main::MoveCamera(float dt)
{
    if (!cameraNode_)
        return;

    // do not move if the UI has a focused element (the console)
    if (GetSubsystem<Urho3D::UI>()->GetFocusElement())
        return;
    
    Urho3D::Input* input = GetSubsystem<Urho3D::Input>();
    
    // movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;
    
    // use this frame's mouse motion to adjust camera node yaw and pitch, clamp the pitch
    // between -90 and 90 degrees
    Urho3D::IntVector2 mouseMove = input->GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Urho3D::Clamp(pitch_, -90.0f, 90.0f);
    
    // construct new orientation for the camera scene node from yaw and pitch, roll is
    // fixed to zero
    cameraNode_->SetRotation(Urho3D::Quaternion(pitch_, yaw_, 0.0f));
    
    // read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown('W'))
        cameraNode_->TranslateRelative(dt * MOVE_SPEED * Urho3D::Vector3::FORWARD);
    if (input->GetKeyDown('S'))
        cameraNode_->TranslateRelative(dt * MOVE_SPEED * Urho3D::Vector3::BACK);
    if (input->GetKeyDown('A'))
        cameraNode_->TranslateRelative(dt * MOVE_SPEED * Urho3D::Vector3::LEFT);
    if (input->GetKeyDown('D'))
        cameraNode_->TranslateRelative(dt * MOVE_SPEED * Urho3D::Vector3::RIGHT);
}

void Main::Update(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    float dt = eventData[Urho3D::Update::P_TIMESTEP].GetFloat();
    MoveCamera(dt);
}

void Main::KeyDown(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData)
{
    int key = eventData[Urho3D::KeyDown::P_KEY].GetInt();
    switch (key)
    {
        case 'Q':
            engine_->Exit();
            break;

        case 'R':
            if (!launchEditor_)
            {
                Urho3D::SharedPtr<Urho3D::File> file = GetSubsystem<Urho3D::ResourceCache>()->
                    GetFile("Data/Scenes/Physics.xml");
                scene_->LoadXML(*file);
            }
            break;

        case Urho3D::KEY_F3:
            GetSubsystem<Urho3D::Console>()->Toggle();
            break;

        case Urho3D::KEY_F6:
            {
                Urho3D::File saveFile(context_, GetSubsystem<Urho3D::FileSystem>()->GetProgramDir()
                        + "/Usr/sav.sav", Urho3D::FILE_WRITE);
                scene_->Save(saveFile);
            }
            break;
        case Urho3D::KEY_F7:
            {
                Urho3D::File saveFile(context_, GetSubsystem<Urho3D::FileSystem>()->GetProgramDir()
                        + "/Usr/sav.sav", Urho3D::FILE_READ);
                scene_->Load(saveFile);
            }
            break;
    }
}

