#ifndef __MAIN_H__
#define __MAIN_H__

#include "Application.h"
#include "Text.h"
#include "Scene.h"
#include "ScriptFile.h"
#include "Context.h"
#include "ResourceCache.h"
#include "Component.h"
#include "PhysicsEvents.h"
#include "RigidBody.h"


// forward declarations
class Editor;

/// main application class
class Main : public Urho3D::Application
{
    OBJECT(Main);

    public:
        /// construct
        Main(Urho3D::Context* context);

        /// setup before engine initialization
        void Setup();
        /// setup after engine initialization and before running the main loop
        void Start();
        /// cleanup after the main loop
        void Stop();


    private:
        // per-frame update event handler
        void Update(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
        // key down event handler
        void KeyDown(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);

        /// handle Camera controls motion
        void MoveCamera(float timeStep);

        bool launchEditor_; // whether to launch the editor
        Urho3D::SharedPtr<Editor> editor_;

        Urho3D::SharedPtr<Urho3D::Scene> scene_;

        Urho3D::SharedPtr<Urho3D::Node> cameraNode_;
        float yaw_;
        float pitch_;
};

#endif

