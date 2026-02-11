int t;  // implicit constant buffer
RWTexture2D<float3> y;

[numthreads(8,8,1)]
void CS(uint3 i:sv_dispatchthreadid) {
    float3 v = i / float3(640, 400, 1) - 1;
    float3 w = normalize(float3(v.x, -v.y * 0.8 - 1, 2));
    float3 p = float3(sin(t*.0044), sin(t*.0024)+2, sin(t*.0034)-5);
    float b = dot(w,p);
    float d = b * b - dot(p,p) + 1;
    float x = 0;
    
    if (d>0) {
        p -= w * (b+sqrt(d));
        x = pow(d,8);
        w = reflect(w,p);
    }
    
    if (w.y<0) {
        p -= w * (p.y+1) / w.y;
        x += (sin(p.z*6) * sin(p.x*6) > 0) * 2 /length(p);
    }
    
    y[i.xy] = abs(v.y-v.x) > 0.1 && abs(v.y+v.x) > 0.1 ? x : float3(1,0,0);
}