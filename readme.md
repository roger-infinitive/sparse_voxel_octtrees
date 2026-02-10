Imports RSVO files and uses a basic greedy mesher to generate exposed faces for each voxel.
Uses a small custom DX11 renderer copied and modified from my game Cultist Astronaut.

This program can also cast rays through the SVO and display intersected nodes via gizmos.

Controls: 
- WASD: Movement
- Space: Ascend
- Shift: Descend
- R: Cast Ray
- C: Clear Gizmos
- ESC: Close Window

How-to Build / Run: 
1. Update config.bat if required. Currently refers to VS2019 vcvarsall.bat.
2. Run 'build'
3. Download RSVO samples from: https://github.com/ephtracy/voxel-model/tree/master/svo
    - TODO(roger): Add a small sample to repo instead of relying on external samples.
4. Put a RSVO file in the data folder and rename it to 'render_me.rsvo'
5. Run svo.exe

Optional:
- If you want to enable the log, you run 'build Debug' and run the exe in a terminal. 
    - This will print the Push, Pop, Advance hierarchy of the Raycast function to the console.

Future:
- USe a mirrored octtree for RaycastSVO to handle negative directions more efficiently.
    - See: https://www.nvidia.com/docs/IO/88972/nvr-2010-001.pdf
- Destroy voxels along a ray.
- Create voxels on a ray point.
- The greedy mesher currently does not merge faces. It only eliminates covered faces.
- IsFilled() could be optimized by building a level-local lookup table instead of relying on SVO masks.
    - lots of popcount calls!
