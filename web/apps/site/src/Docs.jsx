import { Markdown } from '@teslasynth/ui/components/Markdown'

export function DocPage({ content }) {
    return (
        <main class="container">
            <Markdown source={content} />
        </main>
    );
}
