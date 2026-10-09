#!/bin/sh
# build.sh - POSIX shell build script

set -u

[ $# -eq 1 ] || {
	echo "Использование: $0 Исходный файл" >&2
	exit 1
}
src=$1

[ -f "$src" ] && [ -r "$src" ] || {
	echo "Файл не найден: $src" >&2
	exit 2
}

outname=$(sed -n 's/.*Output:[ \t]*\([^ \t]*\).*/\1/p' "$src" | head -n 1)

[ -n "$outname" ] || {
	echo "Не найден комментарий Output: в $src" >&2
	exit 3
}

outname=$(basename "$outname")
src_dir=$(dirname "$src")
tmpdir=

cleanup_exit() {
	rc=$?
	trap - 0 1 2 3 13 15 
	[ -n "$tmpdir" ] && [ -d "$tmpdir" ] && rm -rf -- "$tmpdir"
	exit "$rc"
}

cleanup_signal(){
	sig=$1
	trap - 0 1 2 3 13 15
	[ -n "$tmpdir" ] && [ -d "$tmpdir" ] && rm -rf -- "$tmpdir"
	exit $((128 + sig))
}

trap 'cleanup_exit' 0  
trap 'cleanup_signal 1' 1
trap 'cleanup_signal 2' 2
trap 'cleanup_signal 3' 3
trap 'cleanup_signal 13' 13
trap 'cleanup_signal 15' 15

tmpdir=$(mktemp -d  "${TMPDIR:-/tmp}/build.XXXXXX") || {
	echo "mktemp failed" >&2
	exit 4
}

case "$src" in 
	*.c)
		command -v cc >/dev/null 2>&1 || {
			echo "Компилятор сс не найден" >&2
			exit 8
		}
		cc -o "$tmpdir/$outname"  "$src" || {
			echo "Ошибка компиляции C" >&2
			exit 5
		}
		;;
	*.cc|*.cpp|*.cxx|*.C)
                command -v c++ >/dev/null 2>&1 || { 
		echo "Компилятор C++ не найден" >&2
			 exit 8
		}
		c++ -o "$tmpdir/$outname" "$src" || {
			echo  "Ошибка компиляции C++" >&2
			exit 5
		}
		;;
	*.tex)
                command -v pdflatex >/dev/null 2>&1 || { 
                         echo "Компилятор pdflatex не найден" >&2
                         exit 8
		}
		case "$outname" in 
			*.pdf) : ;;
			*) outname="$outname.pdf";;
		esac
		jobname=${outname%.pdf}

		pdflatex -interaction=nonstopmode -halt-on-error \
			-jobname="$jobname" \
			-output-directory="$tmpdir" "$src" > "$tmpdir/tex.log" 2>&1 || {
				echo "Ошибка компиляции TeX" >&2
				exit 5
		}
		[ -f "$tmpdir/$outname" ] || mv "$tmpdir/$jobname.pdf" "$tmpdir/$outname" || {
			echo "Не удалось получить $outname" >&2
			exit 6
		} 
		;; 
	*)
		echo "Неподдерживаемый тип файла: $src" >&2
		exit 9
		;;
esac

[ -f "$tmpdir/$outname" ] || {
	echo "Конечный файл не создан: $outname" >&2
	exit 6
}
cp -p -- "$tmpdir/$outname" "$src_dir/$outname" || {
	echo "Не удалось скопировать результат" >&2
	exit 7
}

exit 0
