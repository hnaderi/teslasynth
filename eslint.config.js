import js from '@eslint/js';
import reactPlugin from 'eslint-plugin-react';
import reactHooks from 'eslint-plugin-react-hooks';
import tseslint from 'typescript-eslint';
import globals from 'globals';
import prettier from 'eslint-config-prettier';

export default [
    { ignores: ['**/dist/**', '**/node_modules/**'] },

    js.configs.recommended,
    ...tseslint.configs.recommended,

    {
        plugins: {
            react: reactPlugin,
            'react-hooks': reactHooks,
        },
        languageOptions: {
            globals: globals.browser,
            parserOptions: { ecmaFeatures: { jsx: true } },
        },
        settings: {
            react: { version: '18' }, // Preact is API-compatible with React 18
        },
        rules: {
            ...reactPlugin.configs.recommended.rules,
            ...reactHooks.configs.recommended.rules,
            'react/react-in-jsx-scope': 'off',  // not needed with modern JSX transform
            'react/prop-types': 'off',           // not using prop-types
        },
    },

    prettier, // must be last — disables ESLint formatting rules that conflict with Prettier
];
