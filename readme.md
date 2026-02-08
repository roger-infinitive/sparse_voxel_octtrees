Imports RSVO files and uses a basic greedy mesher to generate exposed faces for each voxel.
Uses a small custom DX11 renderer copied and modified from another project.

How to Build / Run: 
1. Update config.bat if required. Currently refers to VS2019 vcvarsall.bat.
2. Run 'build /editor'
3. Download RSVO samples from: https://github.com/ephtracy/voxel-model/tree/master/svo
    - TODO(roger): Add a small sample to repo instead of relying on external samples.
4. Put a RSVO file in the data folder and rename it to 'render_me.rsvo'
5. Run svo.exe

Future Improvements:
- Add a raycaster and destroy voxels along ray.
- The greedy mesher currently does not merge faces. It only eliminates covered faces.
- IsFilled() could be optimized by building a level-local lookup table instead of relying on SVO masks.
    - lots of popcount calls!
