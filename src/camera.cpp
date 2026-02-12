struct Camera {
    Vector3 position;
    
    Vector3 up;
    Vector3 right;
    Vector3 forward;
    
    float yaw;
    float pitch;
    float speed;
};

Matrix4 TickCamera(Camera* camera, Vector2 mouseDelta, float deltaTime) {
    float mouseSensitivity = 1.0f;
    camera->yaw   += mouseDelta.x * mouseSensitivity;
    camera->pitch -= mouseDelta.y * mouseSensitivity;
    
    float pitchLimit = DegreesToRadians(89);    
    camera->pitch = ClampF(camera->pitch, -pitchLimit, pitchLimit);
    
    float cy = cosf(camera->yaw);
    float sy = sinf(camera->yaw);
    float cp = cosf(camera->pitch);
    float sp = sinf(camera->pitch);
    camera->forward = Normalize(Vector3{sy * cp, sp, cy * cp});

    Vector3 worldUp = {0, 1, 0};
    camera->right = Normalize(CrossProduct(worldUp, camera->forward));
    camera->up = CrossProduct(camera->forward, camera->right);
    
    Vector3 movement = {};
    if (IsInputDown(KEY_W)) {
        movement += camera->forward;    
    }
    if (IsInputDown(KEY_A)) {
        movement -= camera->right;    
    }
    if (IsInputDown(KEY_S)) {
        movement -= camera->forward;    
    }
    if (IsInputDown(KEY_D)) {
        movement += camera->right;    
    }
    if (IsInputDown(KEY_SPACE)) {
        movement += camera->up;    
    }
    if (IsInputDown(KEY_SHIFT)) {
        movement -= camera->up;    
    }
    
    movement = Normalize(movement);
    
    float maxCameraSpeed = 2.0f;
    if (SqrMagnitude(movement) < EPSILON) {
        camera->speed -= 10.0f * deltaTime;
    } else {
        camera->speed += 2.0f * deltaTime;
    }
    
    camera->speed = ClampF(camera->speed, 0, maxCameraSpeed);
    camera->position += movement * camera->speed * deltaTime;
    
    Matrix4 view = LookToLH(camera->position, camera->forward, camera->up);
    return view;
}