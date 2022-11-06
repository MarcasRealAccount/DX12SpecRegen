# How to build and use

## Step. 1
Clone [llvm/llvm-project](https://github.com/llvm/llvm-project).  
Run cmake and generate for the clang project.  
Build for debug/release or releaseWithDebugInfo (Sorry for making you have an unusable pc for a few hours xD).  
Install the result (this gives you all the headers necessary).  
Copy the necessary libraries for clangtooling and link them together into ThirdParty/LibTooling/libs/{BuildMode}/LibTooling.lib (BuildMode = Debug, Release (ReleaseWithDebugInfo) or Dist (Release))  
Copy the headers directly into ThirdParty/LibTooling/include/  

## Step. 2
Get yourself some windows sdks and copy the entire include directory into "{CD}/versions/" (Path should be similar to "versions/10.0.10240.0")

## Step. 3
Build and run

## Step. 4
Look at the "{CD}/spec.xml" file