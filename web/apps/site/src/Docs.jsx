import { Markdown } from '@teslasynth/ui/components/Markdown'

export function DocPage({ content }) {
    return (
        <Markdown source={content} />
    );
}
