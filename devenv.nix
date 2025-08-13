{ pkgs, ... }:

{
  env.GREET = "devenv";
  git-hooks.hooks.nixpkgs-fmt.enable = true;

  languages = {
    c.enable = true;
    cplusplus.enable = true;
    shell.enable = true;
  };

  packages = with pkgs; [
    clang
    gnumake
    cmake
    ninja
    gtk3
    gtk4
  ];
}
