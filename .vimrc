set nocompatible
filetype off

set rtp+=~/.vim/bundle/Vundle.vim/
set rtp+=~/.vim/bundle/vim-vue/

call vundle#begin()

Plugin 'VundleVim/Vundle.vim'
Plugin 'posva/vim-vue'

call vundle#end()

filetype plugin indent on
set tabstop=4
set shiftwidth=4
set expandtab
set t_Co=256
syntax on
colorscheme elflord
