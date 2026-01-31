import { useEffect, useRef } from "preact/hooks";
import logoSvgRaw from "../../../../assets/logo.svg?raw";

export function Logo({ size = 48 }) {
    const ref = useRef(null);

    useEffect(() => {
        const parser = new DOMParser();
        const doc = parser.parseFromString(logoSvgRaw, "image/svg+xml");
        const svg = doc.documentElement;

        // Apply external sizing
        svg.style.height = size + "px";
        svg.style.width = "auto";
        svg.setAttribute("preserveAspectRatio", "xMidYMid meet");

        ref.current.appendChild(svg);
    }, []);

    return <div ref={ref} />;
}
