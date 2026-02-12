// so this crashes / freezes when I move the camera to left on start up when origin is 0, 0, 0

StructuredBuffer<uint> Mask       : register(t0);
StructuredBuffer<uint> FirstChild : register(t1);

RWTexture2D<float4> OutColor : register(u0);

cbuffer Params : register(b0) {
    uint ScreenWidth;
    uint ScreenHeight;
    float Fov;
    float Aspect;
    
    uint MaskCount;
    float3 _pad4;
    
    float3 CameraPos;     float _pad0;
    float3 CameraRight;   float _pad1;
    float3 CameraUp;      float _pad2;
    float3 CameraForward; float _pad3;
}

// TODO(roger): Pass into shader.
#define MAX_SVO_DEPTH 13
#define EPSILON 1e-6
#define LARGE_FLOAT 1e30

#define SWAP(a, b) { float _tmp = (a); (a) = (b); (b) = _tmp; }

float copysign(float x, float y) {
    return (y < 0.0) ? -abs(x) : abs(x);
}

struct SvoRayHit {
    uint hit;
    float t;
    float3 normal;
    uint failed;
};

struct SvoStackEntry {
    float3 corner;
    uint mask_idx;
    int idx;
};

SvoRayHit SvoRaycast(float3 ray_start, float3 ray_dir) {
    SvoRayHit out_hit;
    out_hit.hit = 0;
    out_hit.t = 0;
    out_hit.normal = float3(0, 0, 0);
    
    // TODO(roger): remove hardcoded values
    float root_scale = 8.0f;
    
    float3 corner = float3(0, 0, 0);
    float3 upper_corner = corner + float3(root_scale, root_scale, root_scale);
    
    int3 step_dir;
    step_dir.x = (ray_dir.x > 0) ? 1 : ((ray_dir.x < 0) ? -1 : 0);
    step_dir.y = (ray_dir.y > 0) ? 1 : ((ray_dir.y < 0) ? -1 : 0);
    step_dir.z = (ray_dir.z > 0) ? 1 : ((ray_dir.z < 0) ? -1 : 0);
    
    if (abs(ray_dir.x) < EPSILON) { ray_dir.x = copysign(EPSILON, ray_dir.x); }
    if (abs(ray_dir.y) < EPSILON) { ray_dir.y = copysign(EPSILON, ray_dir.y); }
    if (abs(ray_dir.z) < EPSILON) { ray_dir.z = copysign(EPSILON, ray_dir.z); }
    
    float invDx = 1 / ray_dir.x;
    float invDy = 1 / ray_dir.y;
    float invDz = 1 / ray_dir.z;
    
    float3 t_min = float3(-LARGE_FLOAT, -LARGE_FLOAT, -LARGE_FLOAT);
    float3 t_max = float3( LARGE_FLOAT,  LARGE_FLOAT,  LARGE_FLOAT);
    
    // X slab
    if (step_dir.x == 0) {
        if (ray_start.x < corner.x || ray_start.x > upper_corner.x) {
            return out_hit;
        }
    } else {
        t_min.x = invDx * (corner.x - ray_start.x);                
        t_max.x = invDx * (upper_corner.x - ray_start.x);
        if (t_max.x < t_min.x) { 
            SWAP(t_max.x, t_min.x);
        }
    }
    
    // Y slab
    if (step_dir.y == 0) {
        if (ray_start.y < corner.y || ray_start.y > upper_corner.y) {
            return out_hit;
        }
    } else {
        t_min.y = invDy * (corner.y - ray_start.y);                
        t_max.y = invDy * (upper_corner.y - ray_start.y);                
        if (t_max.y < t_min.y) {
            SWAP(t_max.y, t_min.y);
        }
    }
    
    // Z slab
    if (step_dir.z == 0) {
        if (ray_start.z < corner.z || ray_start.z > upper_corner.z) {
            return out_hit;
        }
    } else {
        t_min.z = invDz * (corner.z - ray_start.z);                
        t_max.z = invDz * (upper_corner.z - ray_start.z);                
        if (t_max.z < t_min.z) {
            SWAP(t_max.z, t_min.z);
        }
    }
    
    float t = max(t_min.x, max(t_min.y, t_min.z));
    float t_exit  = min(t_max.x, min(t_max.y, t_max.z));
    t = max(t, 0.0f);

    // Exit if no point along the ray intersects the root bounding box of the SVO.
    if (t_exit < t) {
        return out_hit;
    }
    
    int lvl = 0;
    SvoStackEntry stack[MAX_SVO_DEPTH];
    
    stack[lvl].corner = corner;
    stack[lvl].mask_idx = 0;
    stack[lvl].idx = 0;
    
    float scale = root_scale * 0.5f;
    float3 center = (corner + upper_corner) * 0.5f;
    float3 p = ray_start + (ray_dir * t);
    
    if (p.x >= center.x) { stack[lvl].idx ^= 1; stack[lvl].corner.x += scale; }   
    if (p.y >= center.y) { stack[lvl].idx ^= 2; stack[lvl].corner.y += scale; }   
    if (p.z >= center.z) { stack[lvl].idx ^= 4; stack[lvl].corner.z += scale; }
    
    int last_axis_bit = 0;
    {
        float tx0 = t_min.x;
        float ty0 = t_min.y;
        float tz0 = t_min.z;
        if (tx0 >= ty0 && tx0 >= tz0) last_axis_bit = 1;
        else if (ty0 >= tz0)          last_axis_bit = 2;
        else                          last_axis_bit = 4;
    }
        
    while (true) {        
        upper_corner = stack[lvl].corner + float3(scale, scale, scale);
        uint mask = Mask[stack[lvl].mask_idx] & 0xFFu;
        
        // PUSH
        if (mask & (1u << stack[lvl].idx)) {        
            if (lvl + 1 < MAX_SVO_DEPTH) {
                center = (stack[lvl].corner + upper_corner) * 0.5f;
                scale *= 0.5f;
                
                uint previous_mask_idx = stack[lvl].mask_idx;
                float3 previous_corner = stack[lvl].corner;
                
                uint before_mask = mask & ((1u << stack[lvl].idx) - 1u); 
                
                lvl += 1;
                stack[lvl].corner = previous_corner;
                stack[lvl].mask_idx = FirstChild[previous_mask_idx] + countbits(before_mask);
                stack[lvl].idx = 0;

                if (stack[lvl].mask_idx >= MaskCount) {
                    out_hit.failed = 1;
                    return out_hit;
                }
                
                if (p.x >= center.x) { stack[lvl].idx ^= 1; stack[lvl].corner.x += scale; }
                if (p.y >= center.y) { stack[lvl].idx ^= 2; stack[lvl].corner.y += scale; }
                if (p.z >= center.z) { stack[lvl].idx ^= 4; stack[lvl].corner.z += scale; }
     
                continue;
            } else {
                out_hit.hit = 1;
                out_hit.t = t;
                
                if (last_axis_bit == 1) { out_hit.normal = float3(-step_dir.x, 0, 0); } 
                if (last_axis_bit == 2) { out_hit.normal = float3(0, -step_dir.y, 0); } 
                if (last_axis_bit == 4) { out_hit.normal = float3(0, 0, -step_dir.z); } 
            
                return out_hit;
            }
        }
        
        float tx = LARGE_FLOAT;
        float ty = LARGE_FLOAT;
        float tz = LARGE_FLOAT;
        
        if (step_dir.x != 0) {
            float x = (step_dir.x > 0) ? upper_corner.x : stack[lvl].corner.x;
            tx = (x - ray_start.x) * invDx;
        }
        
        if (step_dir.y != 0) {
            float y = (step_dir.y > 0) ? upper_corner.y : stack[lvl].corner.y;
            ty = (y - ray_start.y) * invDy;
        }
        
        if (step_dir.z != 0) {
            float z = (step_dir.z > 0) ? upper_corner.z : stack[lvl].corner.z;
            tz = (z - ray_start.z) * invDz;
        }
        
        int step_mask = 0;
        if (tx < ty && tx < tz) {
            t = tx;
            step_mask = 1 * step_dir.x;
        } else if (ty < tz) {
            t = ty;
            step_mask = 2 * step_dir.y;
        } else {
            t = tz;
            step_mask = 4 * step_dir.z;
        }
        
        int axis_bit = (step_mask > 0) ? step_mask : -step_mask;
        int is_bit_set = (stack[lvl].idx & axis_bit) != 0;

        // POP
        while ((step_mask > 0 && is_bit_set) || (step_mask < 0 && !is_bit_set)) {
            if (lvl == 0) {
                return out_hit; // exited root bounding box.
            }
            
            scale *= 2;
            lvl -= 1;
            is_bit_set = (stack[lvl].idx & axis_bit) != 0;
        }
        
        stack[lvl].idx += step_mask;
        p = ray_start + (ray_dir * t);
        
        last_axis_bit = axis_bit;
        
        if (axis_bit & 1) { stack[lvl].corner.x += scale * step_dir.x; }
        if (axis_bit & 2) { stack[lvl].corner.y += scale * step_dir.y; }
        if (axis_bit & 4) { stack[lvl].corner.z += scale * step_dir.z; }
    }
    
    return out_hit;
}

void BuildCameraRay(uint2 pix, out float3 ray_start, out float3 ray_dir) {
    float2 uv = (float2(pix) + 0.5) / float2(ScreenWidth, ScreenHeight);
    // Flip Y for screen coords
    float2 ndc = float2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);

    float tan_half_y = tan(0.5 * Fov);
    float tan_half_x = tan_half_y * Aspect;
    float3 dir_view = normalize(float3(ndc.x * tan_half_x, ndc.y * tan_half_y, 1.0f));

    ray_start = CameraPos;
    ray_dir = normalize(dir_view.x * CameraRight + 
                        dir_view.y * CameraUp    + 
                        dir_view.z * CameraForward);
}

[numthreads(8,8,1)]
void CS(uint3 tid : SV_DispatchThreadID) {
    float3 ray_start, ray_dir;
    BuildCameraRay(tid.xy, ray_start, ray_dir);
    
    SvoRayHit ray_hit = SvoRaycast(ray_start, ray_dir);
    
    OutColor[tid.xy] = float4(0,1,0,1);
    if (ray_hit.hit != 0) {
        OutColor[tid.xy] = float4(0.5 * (ray_hit.normal + 1.0), 1.0);
    } else if (ray_hit.failed == 3) {
        OutColor[tid.xy] = float4(1,0,0,1);
    } else {
        OutColor[tid.xy] = float4(0.1,0.1,0.1,1);
    }
}